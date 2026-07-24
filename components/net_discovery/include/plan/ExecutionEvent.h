/**
 * @file ExecutionEvent.h
 * @brief Unified telemetry event payload for execution observers (v6.0 Phase C).
 */

#pragma once

#include "plan/ExecutionState.h"
#include "plan/ExecutionProgress.h"
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Plan {

enum class ExecutionEventType {
    PlanStarted,
    PlanFinished,
    PlanFailed,
    StepStarted,
    StepCompleted,
    StepFailed,
    RollbackStarted,
    RollbackFinished,
    Cancelled
};

struct ExecutionEvent {
    ExecutionEventType type{ExecutionEventType::PlanStarted};
    std::string instanceId;
    std::string planId;
    std::string stepId;
    std::string stepName;
    uint64_t timestampMs{0};
    PlanState planState{PlanState::Pending};
    StepState stepState{StepState::Pending};
    ExecutionProgress progress;
    std::string errorMessage;
};

} // namespace Plan
} // namespace NetDiscovery
