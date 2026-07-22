#include "semantic/SemanticOrchestrator.h"
#include "esp_log.h"
#include <iostream>
#include <thread>

static const char* TAG = "SemanticOrchestrator";

namespace semantic {

SemanticOrchestrator::SemanticOrchestrator(std::shared_ptr<NetDiscovery::ExecutionEngine> executionEngine)
    : m_executionEngine(std::move(executionEngine))
{
}

SemanticError SemanticOrchestrator::Orchestrate(const SemanticRequest& request, 
                                                const std::vector<NetDiscovery::LogicalDevice>& availableDevices,
                                                std::shared_ptr<std::atomic<bool>> cancelToken) {
    // 1. Intent Canonicalization
    NetDiscovery::ActionId canonicalIntent = m_intentCanonicalizer.Normalize(request.rawIntent);
    if (canonicalIntent == NetDiscovery::ActionId::Unknown) {
        return SemanticError::WorkflowGenerationFailed;
    }

    // 2. Device Matching
    auto candidates = m_deviceMatcher.Match(request.targetDescription, availableDevices);
    if (candidates.empty()) {
        return SemanticError::DeviceNotFound;
    }

    // 3. Capability Filtering
    auto validDevices = m_capabilityFilter.Filter(candidates, canonicalIntent);
    if (validDevices.empty()) {
        return SemanticError::MissingCapability;
    }
    if (validDevices.size() > 1) {
        return SemanticError::AmbiguousTarget;
    }

    NetDiscovery::LogicalDevice targetDevice = validDevices.front();

    // 4. Pre-flight Reachability Check
    if (m_executionEngine) {
        NetDiscovery::ExecutionRequest reachReq{targetDevice, {}, {}, {}};
        reachReq.action.id = NetDiscovery::ActionId::CheckReachable;
        auto reachRes = m_executionEngine->Execute(reachReq);
        if (reachRes.status != NetDiscovery::ExecutionStatus::Success) {
            return SemanticError::DeviceUnreachable;
        }
    }

    // 5. Parameter Normalization
    auto params = m_parameterNormalizer.Normalize(request.rawParameters);
    
    // (Optional Entity Resolution - normally we'd replace names with IDs inside params, but we skip for brevity here)

    // 6. Workflow Expansion
    ExecutionPlan plan = m_workflowPlanner.CreatePlan(canonicalIntent, targetDevice, params);
    if (plan.steps.empty()) {
        return SemanticError::WorkflowGenerationFailed;
    }

    ESP_LOGI(TAG, "ActionId       : %s", NetDiscovery::ToString(canonicalIntent).c_str());
    ESP_LOGI(TAG, "Target Device  : %s", targetDevice.displayName.c_str());
    ESP_LOGI(TAG, "Execution Parameters:");
    for (const auto& [k, v] : params) {
        if (std::holds_alternative<std::string>(v)) {
            ESP_LOGI(TAG, "  %s = %s", k.c_str(), std::get<std::string>(v).c_str());
        } else if (std::holds_alternative<int>(v)) {
            ESP_LOGI(TAG, "  %s = %d", k.c_str(), std::get<int>(v));
        } else if (std::holds_alternative<double>(v)) {
            ESP_LOGI(TAG, "  %s = %g", k.c_str(), std::get<double>(v));
        } else if (std::holds_alternative<bool>(v)) {
            ESP_LOGI(TAG, "  %s = %s", k.c_str(), std::get<bool>(v) ? "true" : "false");
        }
    }

    // 7. Orchestration Loop
    for (const auto& step : plan.steps) {
        // Cooperative cancellation check between steps
        if (cancelToken && cancelToken->load()) {
            return SemanticError::Cancelled;
        }

        // Prepare the physical execution request
        NetDiscovery::ExecutionRequest physRequest{targetDevice, {}, {}, {}};
        physRequest.action = step.action;
        // Re-serialize params for ExecutionEngine
        for (const auto& [k, v] : params) {
            if (std::holds_alternative<int>(v)) physRequest.parameters[k] = std::to_string(std::get<int>(v));
            else if (std::holds_alternative<double>(v)) physRequest.parameters[k] = std::to_string(std::get<double>(v));
            else if (std::holds_alternative<bool>(v)) physRequest.parameters[k] = std::get<bool>(v) ? "true" : "false";
            else if (std::holds_alternative<std::string>(v)) physRequest.parameters[k] = std::get<std::string>(v);
        }

        // Wait policy logic BEFORE execution (if we need to wait for a state)
        if (step.waitPolicy == ExecutionWaitPolicy::WaitUntilReachable) {
            bool reachable = false;
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < step.waitTimeout) {
                if (cancelToken && cancelToken->load()) return SemanticError::Cancelled;
                
                bool reachableNow = false;
                if (m_executionEngine) {
                    NetDiscovery::ExecutionRequest reachReq{targetDevice, {}, {}, {}};
                    reachReq.action.id = NetDiscovery::ActionId::CheckReachable;
                    auto reachRes = m_executionEngine->Execute(reachReq);
                    if (reachRes.status == NetDiscovery::ExecutionStatus::Success) {
                        reachableNow = true;
                    }
                } else {
                    reachableNow = true; // Fallback if no engine
                }
                
                if (reachableNow) {
                    reachable = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            if (!reachable && !step.isOptional) {
                return SemanticError::DeviceUnreachable;
            }
        } else if (step.waitPolicy == ExecutionWaitPolicy::FixedDelay) {
            std::this_thread::sleep_for(step.waitTimeout);
        }

        // Fire ExecutionEngine
        if (m_executionEngine) {
            auto result = m_executionEngine->Execute(physRequest);
            if (result.status != NetDiscovery::ExecutionStatus::Success && !step.isOptional) {
                return SemanticError::ExecutionFailed;
            }
        }
    }

    return SemanticError::None;
}

} // namespace semantic
