/**
 * @file ExecutionTransition.h
 * @brief Typed execution transition enum for scheduler-driven control flow (v6.0 Phase D).
 *
 * ExecutionTransition is the exclusive mechanism by which IStepRunner implementations
 * communicate routing decisions to ExecutionScheduler. The scheduler routes graph
 * execution solely via this enum — it never inspects blackboard keys for control flow.
 *
 * Architectural invariant:
 *   - Blackboard (ExecutionPlanContext) stores workflow DATA only.
 *   - ExecutionScheduler makes routing decisions via ExecutionTransition only.
 *   - These two concerns must never be conflated.
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Plan {

enum class ExecutionTransition : uint8_t {
    /// Normal step completion. Scheduler advances using standard Always/OnSuccess edge resolution.
    Continue = 0,

    /// BranchStep predicate evaluated to true. Scheduler activates the true-branch outgoing edge.
    BranchTrue,

    /// BranchStep predicate evaluated to false. Scheduler activates the false-branch outgoing edge.
    BranchFalse,

    /// LoopStep predicate is true and maxIterations not exceeded. Scheduler re-enters the loop body.
    LoopContinue,

    /// LoopStep predicate is false OR maxIterations reached. Scheduler exits the loop to the successor.
    LoopExit,

    /// Runner requests retry. ExecutionOutcome::retryHint carries the retry strategy.
    Retry,

    /// Step was cancelled via CancellationToken.
    Cancel,

    /// Fatal condition. Scheduler aborts the plan immediately without rollback.
    Terminate
};

inline const char* ToString(ExecutionTransition t) {
    switch (t) {
        case ExecutionTransition::Continue:      return "Continue";
        case ExecutionTransition::BranchTrue:    return "BranchTrue";
        case ExecutionTransition::BranchFalse:   return "BranchFalse";
        case ExecutionTransition::LoopContinue:  return "LoopContinue";
        case ExecutionTransition::LoopExit:      return "LoopExit";
        case ExecutionTransition::Retry:         return "Retry";
        case ExecutionTransition::Cancel:        return "Cancel";
        case ExecutionTransition::Terminate:     return "Terminate";
        default:                                 return "Unknown";
    }
}

} // namespace Plan
} // namespace NetDiscovery
