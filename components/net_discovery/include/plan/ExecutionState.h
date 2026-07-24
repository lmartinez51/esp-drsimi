/**
 * @file ExecutionState.h
 * @brief Lifecycle states for ExecutionPlanInstance and IExecutionStep (v6.0 Phase C).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Plan {

enum class PlanState {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled,
    RollingBack,
    RolledBack
};

enum class StepState {
    Pending,
    Running,
    Succeeded,
    Failed,
    Skipped,
    Cancelled,
    RolledBack
};

inline std::string ToString(PlanState state) {
    switch (state) {
        case PlanState::Pending: return "Pending";
        case PlanState::Running: return "Running";
        case PlanState::Completed: return "Completed";
        case PlanState::Failed: return "Failed";
        case PlanState::Cancelled: return "Cancelled";
        case PlanState::RollingBack: return "RollingBack";
        case PlanState::RolledBack: return "RolledBack";
        default: return "Unknown";
    }
}

inline std::string ToString(StepState state) {
    switch (state) {
        case StepState::Pending: return "Pending";
        case StepState::Running: return "Running";
        case StepState::Succeeded: return "Succeeded";
        case StepState::Failed: return "Failed";
        case StepState::Skipped: return "Skipped";
        case StepState::Cancelled: return "Cancelled";
        case StepState::RolledBack: return "RolledBack";
        default: return "Unknown";
    }
}

} // namespace Plan
} // namespace NetDiscovery
