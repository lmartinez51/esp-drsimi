/**
 * @file ExecutionInfrastructure.cpp
 * @brief Implementation of ExecutionInfrastructure (v5.1.0 Phase B).
 */

#include "ExecutionInfrastructure.h"
#include "esp_log.h"
#include <thread>

static const char* TAG = "ExecutionInfrastructure";

namespace NetDiscovery {

ExecutionInfrastructure::ExecutionInfrastructure(std::shared_ptr<ExecutionEngine> executionEngine)
    : m_executionEngine(std::move(executionEngine))
{
}

ExecutionResult ExecutionInfrastructure::ExecuteWithPolicy(const BoundExecutionRequest& request) {
    if (!m_executionEngine) {
        ExecutionResult res;
        res.status = ExecutionStatus::ExecutionFailed;
        res.errorMessage = "Missing ExecutionEngine in ExecutionInfrastructure.";
        return res;
    }

    if (!request.targetDevice) {
        ExecutionResult res;
        res.status = ExecutionStatus::ExecutionFailed;
        res.errorMessage = "Missing target device in ExecutionInfrastructure.";
        return res;
    }

    const LogicalDevice& device = *request.targetDevice;
    const ExecutionPolicy& policy = request.policy;

    // 1. Policy-Driven Reachability Verification
    if (policy.GetReachabilityStrategy() != ReachabilityStrategy::None) {
        auto verifier = ReachabilityVerifierFactory::CreateVerifier(device);
        bool isReachable = false;

        if (policy.GetReachabilityStrategy() == ReachabilityStrategy::VerifyOnce) {
            isReachable = verifier->Verify(device, nullptr, policy.GetOptions().reachabilityTimeoutMs);
        } else if (policy.GetReachabilityStrategy() == ReachabilityStrategy::WaitUntilReachable) {
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < policy.GetOptions().reachabilityTimeoutMs) {
                if (verifier->Verify(device, nullptr, std::chrono::milliseconds(500))) {
                    isReachable = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        if (!isReachable) {
            ESP_LOGW(TAG, "Reachability verification failed for device '%s'", device.displayName.c_str());
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Device reachability policy check failed for " + device.displayName;
            return res;
        }
    }

    // 2. Retry Loop & Execution Dispatch
    int retriesLeft = policy.GetOptions().maxRetries;
    ExecutionResult result;

    while (true) {
        result = m_executionEngine->Execute(request);
        if (result.status == ExecutionStatus::Success || retriesLeft <= 0) {
            break;
        }
        ESP_LOGW(TAG, "Execution failed (status=%s). Retrying (%d retries left)...",
                 ToString(result.status).c_str(), retriesLeft);
        retriesLeft--;
        std::this_thread::sleep_for(policy.GetOptions().retryBackoffMs);
    }

    return result;
}

} // namespace NetDiscovery
