/**
 * @file ExecutionPolicy.h
 * @brief Strategy policy metadata for execution planning (v5.0.0 Architecture Phase 8.5).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Planner strategy execution mode enum.
 */
enum class ExecutionMode {
    FailFast,             // Abort plan immediately upon first step failure
    ContinueOnError,      // Continue remaining non-dependent steps on step failure
    RollbackOnFailure,    // Trigger registered rollback steps on step failure
    RetryFailedSteps,     // Automatically retry failed steps up to maxRetries
    RequireConfirmation,  // Halt for UI user confirmation before execution
    DryRun,               // Validate plan without dispatching to adapters
    Simulation            // Emulate execution latency without physical I/O
};

/**
 * @brief String conversion helper for ExecutionMode.
 */
inline std::string ToString(ExecutionMode mode) {
    switch (mode) {
        case ExecutionMode::FailFast:            return "FailFast";
        case ExecutionMode::ContinueOnError:     return "ContinueOnError";
        case ExecutionMode::RollbackOnFailure:   return "RollbackOnFailure";
        case ExecutionMode::RetryFailedSteps:    return "RetryFailedSteps";
        case ExecutionMode::RequireConfirmation: return "RequireConfirmation";
        case ExecutionMode::DryRun:              return "DryRun";
        case ExecutionMode::Simulation:          return "Simulation";
        default:                                 return "Unknown";
    }
}

/**
 * @brief Pure metadata container defining execution behavior policies.
 */
struct ExecutionPolicy {
    ExecutionMode mode{ExecutionMode::FailFast};                     // Execution strategy mode
    uint8_t maxRetries{3};                                            // Maximum automatic retry attempts per step
    bool stopOnFailure{true};                                         // True if plan halts on step failure
    bool allowParallel{true};                                         // True if steps with matching parallelGroup execute concurrently
    uint32_t overallTimeoutMs{15000};                                 // Maximum overall plan execution timeout
    std::unordered_map<std::string, std::string> metadata;          // Extensible policy metadata

    ExecutionPolicy() = default;

    ExecutionPolicy(ExecutionMode m, uint8_t retries = 3, bool stopFail = true)
        : mode(m), maxRetries(retries), stopOnFailure(stopFail) {}
};

} // namespace Execution
} // namespace NetDiscovery
