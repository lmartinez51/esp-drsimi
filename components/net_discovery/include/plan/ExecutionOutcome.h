/**
 * @file ExecutionOutcome.h
 * @brief Single execution contract between IStepRunner and ExecutionScheduler (v6.0 Phase D).
 *
 * ExecutionOutcome is the canonical return type of IStepRunner::RunStep.
 * It carries the complete, structured result of a step execution including:
 *   - status:          coarse-grained outcome
 *   - transition:      typed routing signal (consumed exclusively by ExecutionScheduler)
 *   - outputPayload:   primary typed output (written to blackboard by IBindingResolver)
 *   - diagnostics:     optional timing and trace data
 *   - retryHint:       retry strategy (populated when transition == Retry)
 *   - rollbackMetadata: rollback context for IRollbackCapable coordination
 */

#pragma once

#include "plan/ExecutionTransition.h"
#include "plan/ExecutionValue.h"
#include "core/ExecutionResult.h"
#include <optional>
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Plan {

// ---------------------------------------------------------------------------
// StepDiagnostics — optional timing and trace data produced by a runner
// ---------------------------------------------------------------------------
struct StepDiagnostics {
    std::string nodeId;
    std::string stepName;
    int64_t     startTimeMs{0};
    int64_t     endTimeMs{0};
    int64_t     elapsedMs{0};
};

// ---------------------------------------------------------------------------
// RetryHint — retry strategy requested by a runner (transition == Retry)
// ---------------------------------------------------------------------------
struct RetryHint {
    uint8_t  maxAttempts{1};
    uint32_t delayMs{0};
    float    backoffMultiplier{1.0f};
};

// ---------------------------------------------------------------------------
// RollbackMetadata — rollback context for IRollbackCapable coordination
// Opaque to the runtime; meaningful only to the step that produced it and
// the CompositeStepRunner that consumes it during RollbackOnFailure.
// ---------------------------------------------------------------------------
struct RollbackMetadata {
    std::string snapshotKey;
    std::string rollbackToken;
};

// ---------------------------------------------------------------------------
// ExecutionOutcome — single runner→scheduler contract
// ---------------------------------------------------------------------------
struct ExecutionOutcome {
    /// Coarse-grained execution status.
    ExecutionResult status;

    /// Typed routing signal. Default: Continue (advance by standard edge resolution).
    ExecutionTransition transition{ExecutionTransition::Continue};

    /// Primary typed output of this step.
    /// Written to the blackboard by IBindingResolver::PropagateOutput after RunStep returns.
    std::optional<ExecutionValue> outputPayload;

    /// Optional structured diagnostics (timing, step identity).
    std::optional<StepDiagnostics> diagnostics;

    /// Optional retry strategy. Populated when transition == Retry.
    std::optional<RetryHint> retryHint;

    /// Optional rollback context for IRollbackCapable implementations.
    std::optional<RollbackMetadata> rollbackMetadata;

    // -----------------------------------------------------------------------
    // Static factories
    // -----------------------------------------------------------------------

    static ExecutionOutcome Success() {
        ExecutionOutcome o;
        o.status.status = ExecutionStatus::Success;
        o.transition    = ExecutionTransition::Continue;
        return o;
    }

    static ExecutionOutcome Failure(ExecutionResult reason) {
        ExecutionOutcome o;
        o.status     = reason;
        o.transition = ExecutionTransition::Continue;
        return o;
    }

    static ExecutionOutcome WithTransition(ExecutionResult s, ExecutionTransition t) {
        ExecutionOutcome o;
        o.status     = s;
        o.transition = t;
        return o;
    }

    static ExecutionOutcome Cancelled() {
        ExecutionOutcome o;
        o.status.status = ExecutionStatus::ExecutionFailed;
        o.status.errorMessage = "Cancelled";
        o.transition = ExecutionTransition::Cancel;
        return o;
    }

    static ExecutionOutcome Terminate(ExecutionResult reason) {
        ExecutionOutcome o;
        o.status     = reason;
        o.transition = ExecutionTransition::Terminate;
        return o;
    }

    bool IsSuccess() const {
        return status.status == ExecutionStatus::Success;
    }
};

} // namespace Plan
} // namespace NetDiscovery
