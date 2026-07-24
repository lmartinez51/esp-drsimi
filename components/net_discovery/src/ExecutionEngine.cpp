#include "ExecutionEngine.h"
#include "esp_log.h"
#include <iostream>
#include <chrono>

static const char* TAG = "ExecutionEngine";

namespace NetDiscovery {

ExecutionEngine::ExecutionEngine(const TransportRegistry& transportRegistry, 
                                 const ControllerRegistry& controllerRegistry,
                                 std::shared_ptr<AuthenticationManager> authManager)
    : transportSelector(transportRegistry), controllerRegistry(controllerRegistry), authManager(authManager) {
}

ExecutionResult ExecutionEngine::Execute(const BoundExecutionRequest& request) {
    auto startTime = std::chrono::steady_clock::now();

    if (!request.targetDevice) {
        ExecutionResult res;
        res.status = ExecutionStatus::ExecutionFailed;
        res.errorMessage = "Missing target device in BoundExecutionRequest.";
        return res;
    }

    const LogicalDevice& device = *request.targetDevice;
    IDeviceController* controller = request.selectedController;

    if (!controller) {
        auto& controllers = controllerRegistry.GetControllers();
        for (const auto& candidate : device.controllerCandidates) {
            if (candidate.isRejected) continue;
            for (const auto& c : controllers) {
                if (c->ControllerName() == candidate.name) {
                    controller = c.get();
                    break;
                }
            }
            if (controller) break;
        }
    }

    if (!controller) {
        ExecutionResult res;
        res.status = ExecutionStatus::ExecutionFailed;
        res.errorMessage = "No accepted controller available for device: " + device.displayName;
        return res;
    }

    // Route generation owned by IDeviceController
    auto routeOpt = controller->GetExecutionRoute(device, request.action);
    if (!routeOpt.has_value()) {
        ExecutionResult res;
        res.status = ExecutionStatus::UnsupportedAction;
        res.errorMessage = "Selected controller '" + controller->ControllerName() + "' failed route generation for action: " + ToString(request.action.id);
        auto endTime = std::chrono::steady_clock::now();
        res.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return res;
    }

    ExecutionRoute route = routeOpt.value();

    ESP_LOGI(TAG, "========== EXECUTION ROUTE GENERATION DUMP ==========");
    ESP_LOGI(TAG, "Target Entity : %s", device.displayName.c_str());
    ESP_LOGI(TAG, "Action        : %s", ToString(request.action.id).c_str());
    ESP_LOGI(TAG, "Capabilities  : %d enumerated", (int)device.capabilities.size());
    ESP_LOGI(TAG, "Controller    : %s", controller->ControllerName().c_str());
    ESP_LOGI(TAG, "Transport     : %s", ToString(route.transport).c_str());
    ESP_LOGI(TAG, "Metadata Count: %d", (int)route.metadata.size());
    for (const auto& [k, v] : route.metadata) {
        ESP_LOGI(TAG, "  Metadata [%s] = %s", k.c_str(), v.c_str());
    }
    ESP_LOGI(TAG, "=====================================================");

    // Resolve transport
    auto transport = transportSelector.SelectTransport(route);
    if (!transport) {
        ExecutionResult res;
        res.status = ExecutionStatus::TransportUnavailable;
        res.errorMessage = "No transport found for family: " + ToString(route.transport);
        auto endTime = std::chrono::steady_clock::now();
        res.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return res;
    }

    // Build ExecutionRequest wrapper for strategy & transport
    ExecutionRequest physRequest{device, request.action, request.parameters, request.context};

    // Authentication credential injection
    if (authManager) {
        authManager->InjectCredentials(physRequest.device.id, physRequest.context);
        std::string deviceId = physRequest.device.id;
        std::shared_ptr<AuthenticationManager> localAuthManager = authManager;
        physRequest.context.onCredentialUpdated = [deviceId, localAuthManager](const std::string& key, const std::string& value) {
            localAuthManager->SaveCredentials(deviceId, key, value);
        };
    }

    // Strategy request building
    if (route.strategy) {
        route.strategy->BuildRequest(physRequest, route);
    }

    // Transport execution
    ExecutionResult result = transport->Execute(physRequest, route);

    // Strategy response processing
    if (route.strategy) {
        route.strategy->ProcessResponse(result, physRequest.context);
    }

    if (result.elapsedTimeMs == 0) {
        auto endTime = std::chrono::steady_clock::now();
        result.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    }

    return result;
}

} // namespace NetDiscovery
