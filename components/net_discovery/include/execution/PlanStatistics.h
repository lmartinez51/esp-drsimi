/**
 * @file PlanStatistics.h
 * @brief Architectural metadata metrics computed during execution planning (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include <cstdint>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Pure metadata container capturing static execution plan metrics.
 */
struct PlanStatistics {
    uint32_t estimatedDurationMs{0};            // Estimated overall execution duration
    uint32_t estimatedNetworkOperations{0};      // Expected network packet transactions
    uint32_t estimatedProtocolTransitions{0};    // Unique protocol adapters involved
    uint32_t parallelGroups{0};                  // Count of distinct parallel execution groups
    uint32_t criticalPathLength{0};              // Step count along the longest execution dependency chain
    uint32_t maximumDepth{0};                    // Deepest step depth in the DAG
    uint32_t rollbackSteps{0};                  // Count of registered rollback steps
    uint32_t optionalSteps{0};                  // Count of optional steps

    PlanStatistics() = default;
};

} // namespace Execution
} // namespace NetDiscovery
