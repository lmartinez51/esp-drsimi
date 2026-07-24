/**
 * @file RuntimeConfiguration.h
 * @brief Immutable runtime construction parameters for RuntimeExecutionEngine (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

#include <cstdint>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Immutable value object carrying all tuneable parameters for RuntimeExecutionEngine.
 *
 * Passed at construction time. No setters. No hardcoded constants inside the engine.
 * Callers that do not need custom behaviour construct with RuntimeConfiguration::Default().
 */
struct RuntimeConfiguration {
    // ── Timing ─────────────────────────────────────────────────────────────
    uint32_t defaultTimeoutMs{30'000};      ///< Per-step timeout (ms) when not overridden by step policy.
    uint32_t sessionTimeoutMs{300'000};     ///< Maximum total session duration (ms) before forced Timeout.

    // ── Retry ──────────────────────────────────────────────────────────────
    uint32_t maximumRetries{3};             ///< Maximum step retry attempts before marking step Failed.
    uint32_t retryBackoffBaseMs{500};       ///< Base backoff delay multiplied by retry count.

    // ── Scheduler ──────────────────────────────────────────────────────────
    uint32_t maxConcurrentSteps{4};         ///< Upper bound of steps dispatched within a single Tick().
    uint32_t schedulerBatchSize{8};         ///< Maximum steps evaluated per scheduling pass.

    // ── Queue ──────────────────────────────────────────────────────────────
    uint32_t eventQueueCapacity{64};        ///< Maximum events buffered in ExecutionEventQueue per session.

    // ── Observability ──────────────────────────────────────────────────────
    bool diagnosticsEnabled{true};          ///< Enable RuntimeDiagnostics counter accumulation.
    bool tracingEnabled{false};             ///< Enable per-step execution tracing (verbose).

    // ── Factory ────────────────────────────────────────────────────────────
    /**
     * @brief Returns a RuntimeConfiguration populated with reasonable production defaults.
     */
    static RuntimeConfiguration Default() {
        return RuntimeConfiguration{};
    }

    /**
     * @brief Returns a RuntimeConfiguration optimised for embedded targets with tight RAM budgets.
     */
    static RuntimeConfiguration Embedded() {
        RuntimeConfiguration cfg;
        cfg.maximumRetries         = 1;
        cfg.maxConcurrentSteps     = 2;
        cfg.schedulerBatchSize     = 4;
        cfg.eventQueueCapacity     = 16;
        cfg.diagnosticsEnabled     = false;
        cfg.tracingEnabled         = false;
        return cfg;
    }
};

} // namespace Runtime
} // namespace NetDiscovery
