/**
 * @file ValidationAssertionHelpers.h
 * @brief Reusable assertion helpers for validating test scenario outcomes (v5.0.0 Architecture Phase 16).
 */

#pragma once

#include "runtime/ExecutionStepResult.h"
#include "execution/ExecutionSession.h"
#include "testing/RuntimeTestHarness.h"

#include <string>
#include <vector>

namespace NetDiscovery {
namespace Testing {

/**
 * @brief Helper assertion functions validating execution order, status, and telemetry.
 */
class ValidationAssertionHelpers {
public:
    static bool AssertSessionCompleted(Execution::ExecutionPlanState state) {
        return state == Execution::ExecutionPlanState::Completed;
    }

    static bool AssertSessionFailed(Execution::ExecutionPlanState state) {
        return state == Execution::ExecutionPlanState::Failed;
    }

    static bool AssertStepSuccess(const Runtime::ExecutionStepResult& result) {
        return result.status == Execution::StepStatus::Success;
    }

    static bool AssertCapabilityMismatch(const Runtime::ExecutionStepResult& result) {
        return result.failureReason == Runtime::ExecutionFailureReason::CapabilityMismatch;
    }

    static bool AssertTimeout(const Runtime::ExecutionStepResult& result) {
        return result.status == Execution::StepStatus::Timeout;
    }
};

} // namespace Testing
} // namespace NetDiscovery
