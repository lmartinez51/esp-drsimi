/**
 * @file ExecutionCapabilities.h
 * @brief Descriptor representing platform capabilities supported by the execution layer (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Immutable platform capability flags describing what features the underlying execution engine supports.
 */
struct ExecutionCapabilities {
    bool supportsRollback{true};            // Supports executing rollback steps on failure
    bool supportsParallel{true};            // Supports parallel step scheduling
    bool supportsBatchExecution{true};       // Supports batching operations
    bool supportsTransactions{false};       // Supports atomic multi-step rollback transactions
    bool supportsDryRun{true};              // Supports dry-run execution validation
    bool supportsSimulation{true};          // Supports simulation latency testing
    bool supportsPartialCompletion{true};   // Supports completing partial steps on non-fatal failures
    bool supportsCancellation{true};       // Supports plan abort/cancellation

    ExecutionCapabilities() = default;
};

} // namespace Execution
} // namespace NetDiscovery
