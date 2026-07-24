/**
 * @file ExecutionPlanState.h
 * @brief Strongly typed execution lifecycle states for ExecutionSession (v5.0.0 Architecture Phase 9).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Strongly typed lifecycle states for runtime execution engine sessions.
 */
enum class ExecutionPlanState {
    Created,       // Session created, initialized
    Ready,         // Ready for step scheduling
    Running,       // Steps currently executing
    Waiting,       // Waiting for step response or dependency
    Paused,        // User or system paused execution
    Completed,     // All steps completed successfully
    Failed,        // Execution failed fatally
    Cancelled,     // Session explicitly cancelled
    Rollback,      // Failure triggered rollback request
    RollingBack,   // Executing rollback steps
    RolledBack,    // Rollback steps completed successfully
    Timeout        // Session overall execution timed out
};

inline std::string ToString(ExecutionPlanState state) {
    switch (state) {
        case ExecutionPlanState::Created:     return "Created";
        case ExecutionPlanState::Ready:       return "Ready";
        case ExecutionPlanState::Running:     return "Running";
        case ExecutionPlanState::Waiting:     return "Waiting";
        case ExecutionPlanState::Paused:      return "Paused";
        case ExecutionPlanState::Completed:   return "Completed";
        case ExecutionPlanState::Failed:      return "Failed";
        case ExecutionPlanState::Cancelled:   return "Cancelled";
        case ExecutionPlanState::Rollback:    return "Rollback";
        case ExecutionPlanState::RollingBack: return "RollingBack";
        case ExecutionPlanState::RolledBack:  return "RolledBack";
        case ExecutionPlanState::Timeout:     return "Timeout";
        default:                              return "Unknown";
    }
}

} // namespace Execution
} // namespace NetDiscovery
