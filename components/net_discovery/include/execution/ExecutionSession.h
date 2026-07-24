/**
 * @file ExecutionSession.h
 * @brief Representation of active runtime execution state for an ExecutionPlan (v5.0.0 Architecture Phase 9.1).
 */

#pragma once

#include "execution/ExecutionPlanState.h"
#include "execution/ExecutionStepResult.h"
#include "execution/ExecutionPlannerTypes.h"
#include "execution/ExecutionCursor.h"
#include "runtime/ExecutionRuntimeContext.h"
#include "runtime/ExecutionEventQueue.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

using SessionId = std::string;

/**
 * @brief Mutable runtime container holding the active execution lifecycle state of a plan.
 */
struct ExecutionSession {
    SessionId sessionId;
    std::string planId;
    RequestId requestId;
    ExecutionPlanState currentState{ExecutionPlanState::Created};
    std::optional<StepId> currentStepId{std::nullopt};
    std::vector<StepId> completedSteps;
    std::vector<StepId> failedSteps;
    std::vector<StepId> pendingSteps;
    uint64_t startTimestampMs{0};
    uint64_t endTimestampMs{0};
    std::unordered_map<StepId, ExecutionStepResult> stepResults;
    std::unordered_map<std::string, std::string> metadata;

    // Phase 9.1 Runtime Components Owned Exclusively by ExecutionSession
    Runtime::ExecutionRuntimeContext runtimeContext;
    Runtime::ExecutionEventQueue eventQueue;
    ExecutionCursor cursor;

    ExecutionSession() = default;

    ExecutionSession(SessionId sId, std::string pId, RequestId rId, uint64_t startMs = 0)
        : sessionId(std::move(sId)), planId(std::move(pId)), requestId(std::move(rId)),
          currentState(ExecutionPlanState::Created), startTimestampMs(startMs) {}

    bool IsActive() const {
        return currentState == ExecutionPlanState::Ready ||
               currentState == ExecutionPlanState::Running ||
               currentState == ExecutionPlanState::Waiting ||
               currentState == ExecutionPlanState::RollingBack;
    }

    bool IsTerminal() const {
        return currentState == ExecutionPlanState::Completed ||
               currentState == ExecutionPlanState::Failed ||
               currentState == ExecutionPlanState::Cancelled ||
               currentState == ExecutionPlanState::RolledBack ||
               currentState == ExecutionPlanState::Timeout;
    }
};

} // namespace Execution
} // namespace NetDiscovery
