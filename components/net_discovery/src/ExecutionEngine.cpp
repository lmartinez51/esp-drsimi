#include "ExecutionEngine.h"
#include <iostream>
#include <chrono>

namespace NetDiscovery {

ExecutionEngine::ExecutionEngine(const TransportRegistry& transportRegistry, 
                                 const ControllerRegistry& controllerRegistry,
                                 std::shared_ptr<AuthenticationManager> authManager)
    : transportSelector(transportRegistry), controllerRegistry(controllerRegistry), authManager(authManager) {
}

ExecutionResult ExecutionEngine::Execute(const ExecutionRequest& request) {
    auto startTime = std::chrono::steady_clock::now();
    
    // 1 & 2. Find a controller that supports the requested action
    if (request.device.controllerCandidates.empty()) {
        ExecutionResult res;
        res.status = ExecutionStatus::ExecutionFailed;
        res.errorMessage = "No controllers available for this device.";
        return res;
    }
    
    auto& controllers = controllerRegistry.GetControllers();
    std::optional<ExecutionRoute> routeOpt = std::nullopt;

    for (const auto& candidate : request.device.controllerCandidates) {
        if (candidate.isRejected) continue; // Skip rejected candidates

        IDeviceController* activeController = nullptr;
        for (const auto& c : controllers) {
            if (c->ControllerName() == candidate.name) {
                activeController = c.get();
                break;
            }
        }

        if (activeController) {
            routeOpt = activeController->GetExecutionRoute(request.device, request.action);
            if (routeOpt.has_value()) {
                std::cout << "  [ExecutionEngine] Controller '" << activeController->ControllerName() << "' returned a route.\n";
                std::cout << "  [ExecutionEngine]   -> TransportFamily: " << ToString(routeOpt->transport) << "\n";
                std::cout << "  [ExecutionEngine]   -> Metadata keys: " << routeOpt->metadata.size() << "\n";
                if (routeOpt->preferredEndpoint) {
                    std::cout << "  [ExecutionEngine]   -> Preferred Endpoint IP: " << routeOpt->preferredEndpoint->ip << "\n";
                    if (routeOpt->preferredEndpoint->evidence.upnp.has_value()) {
                        std::cout << "  [ExecutionEngine]      - UPnP Location: " << routeOpt->preferredEndpoint->evidence.upnp->locationUrl << "\n";
                    }
                }
                break; // Found a controller that supports the action
            }
        }
    }

    if (!routeOpt.has_value()) {
        ExecutionResult res;
        res.status = ExecutionStatus::UnsupportedAction;
        res.errorMessage = "No accepted controller supports the requested action.";
        auto endTime = std::chrono::steady_clock::now();
        res.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return res;
    }

    // 3. Resolve the transport
    auto transport = transportSelector.SelectTransport(routeOpt.value());
    if (!transport) {
        ExecutionResult res;
        res.status = ExecutionStatus::TransportUnavailable;
        res.errorMessage = "No transport found for family: " + ToString(routeOpt.value().transport);
        auto endTime = std::chrono::steady_clock::now();
        res.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return res;
    }

    // 3.5. Authentication
    ExecutionRequest modifiableRequest = request; // In real code, request shouldn't be const if we mutate context, but for now we just cast or copy
    if (authManager) {
        authManager->InjectCredentials(modifiableRequest.device.id, modifiableRequest.context);
        
        std::string deviceId = modifiableRequest.device.id;
        std::shared_ptr<AuthenticationManager> localAuthManager = authManager;
        modifiableRequest.context.onCredentialUpdated = [deviceId, localAuthManager](const std::string& key, const std::string& value) {
            localAuthManager->SaveCredentials(deviceId, key, value);
        };
    }

    // 3.8. Strategy Request Building
    if (routeOpt.value().strategy) {
        routeOpt.value().strategy->BuildRequest(modifiableRequest, routeOpt.value());
    }

    // 4. Dispatch
    ExecutionResult result = transport->Execute(modifiableRequest, routeOpt.value());
    
    // 5. Strategy Response Processing
    if (routeOpt.value().strategy) {
        routeOpt.value().strategy->ProcessResponse(result, modifiableRequest.context);
    }
    

    
    // In case the transport didn't fill in elapsed time
    if (result.elapsedTimeMs == 0) {
        auto endTime = std::chrono::steady_clock::now();
        result.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    }

    return result;
}

} // namespace NetDiscovery
