/**
 * @file DispatcherCapabilities.h
 * @brief Static capability descriptor for IExecutionDispatcher implementations (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Value object describing the capabilities a concrete dispatcher exposes.
 *
 * Returned by IExecutionDispatcher::GetCapabilities(). The runtime and planner
 * query these flags at session start to validate that selected bindings are
 * compatible with the attached dispatcher before dispatching a single step.
 *
 * No protocol logic. No execution. Pure metadata.
 */
struct DispatcherCapabilities {
    // ── Core Dispatcher Support ─────────────────────────────────────────────
    bool supportsRollback{false};           ///< Dispatcher can execute rollback steps on failure.
    bool supportsBatchExecution{false};     ///< Dispatcher can process multiple steps in one call.
    bool supportsParallelExecution{false};  ///< Dispatcher can execute independent steps concurrently.
    bool supportsTransactions{false};       ///< Dispatcher wraps multiple steps in a single atomic transaction.
    bool supportsStreaming{false};          ///< Dispatcher supports streaming step outputs incrementally.
    bool supportsCancellation{false};       ///< Dispatcher can honour mid-step cancellation requests.
    bool supportsTimeouts{false};           ///< Dispatcher enforces per-step timeout thresholds.

    // ── Capability Queries ──────────────────────────────────────────────────
    bool CanRollback()       const { return supportsRollback; }
    bool CanBatch()          const { return supportsBatchExecution; }
    bool CanRunInParallel()  const { return supportsParallelExecution; }
    bool CanTransact()       const { return supportsTransactions; }
    bool CanStream()         const { return supportsStreaming; }
    bool CanCancel()         const { return supportsCancellation; }
    bool CanEnforceTimeout() const { return supportsTimeouts; }

    // ── Factory Helpers ─────────────────────────────────────────────────────
    /**
     * @brief Returns a fully capable descriptor (all flags true). Used for testing.
     */
    static DispatcherCapabilities Full() {
        return {true, true, true, true, true, true, true};
    }

    /**
     * @brief Returns a minimal descriptor (all flags false). Correct for NullExecutionDispatcher.
     */
    static DispatcherCapabilities None() {
        return {};
    }

    /**
     * @brief Returns a descriptor typical of a basic synchronous protocol adapter.
     */
    static DispatcherCapabilities BasicSynchronous() {
        DispatcherCapabilities c;
        c.supportsCancellation = true;
        c.supportsTimeouts     = true;
        return c;
    }
};

} // namespace Runtime
} // namespace NetDiscovery
