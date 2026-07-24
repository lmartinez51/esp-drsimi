/**
 * @file StepExecutionState.h
 * @brief Mutable per-execution runtime state for a single step invocation (v6.0 Phase D).
 *
 * Follows the same design pattern as ExecutionPlan (immutable descriptor) vs.
 * ExecutionPlanInstance (mutable runtime state). Every IExecutionStep is a pure
 * immutable descriptor; StepExecutionState holds all mutable per-execution data.
 *
 * Owned by ExecutionPlanInstance, indexed by stepId.
 */

#pragma once

#include "plan/ExecutionState.h"
#include "plan/ExecutionTransition.h"
#include "plan/ExecutionOutcome.h"
#include <string>
#include <optional>
#include <cstdint>

namespace NetDiscovery {
namespace Plan {

struct StepExecutionState {
    /// The step this state belongs to.
    std::string stepId;

    /// Current lifecycle state (Pending, Running, Succeeded, Failed, ...).
    StepState state{StepState::Pending};

    /// Number of execution attempts (tracks retries and loop iterations).
    uint8_t attemptCount{0};

    /// Timestamp when the most recent RunStep call began (ms from IRuntimeClock).
    int64_t startTimeMs{0};

    /// Timestamp when the most recent RunStep call completed (ms from IRuntimeClock).
    int64_t endTimeMs{0};

    /// The transition returned by the most recent RunStep call.
    ExecutionTransition lastTransition{ExecutionTransition::Continue};

    /// Full outcome of the most recent RunStep call.
    std::optional<ExecutionOutcome> lastOutcome;

    // -----------------------------------------------------------------------
    // Convenience
    // -----------------------------------------------------------------------
    bool IsPending()   const { return state == StepState::Pending;    }
    bool IsRunning()   const { return state == StepState::Running;     }
    bool IsSucceeded() const { return state == StepState::Succeeded;   }
    bool IsFailed()    const { return state == StepState::Failed;      }
    bool IsTerminal()  const {
        return state == StepState::Succeeded ||
               state == StepState::Failed    ||
               state == StepState::Cancelled ||
               state == StepState::RolledBack;
    }
};

} // namespace Plan
} // namespace NetDiscovery
