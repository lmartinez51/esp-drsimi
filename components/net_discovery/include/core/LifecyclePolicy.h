/**
 * @file LifecyclePolicy.h
 * @brief Injectable policy defining temporal thresholds for entity state transitions.
 */

#pragma once

#include <chrono>

namespace NetDiscovery {

/**
 * @brief Injectable policy defining temporal thresholds for entity state transitions.
 */
struct LifecyclePolicy {
    std::chrono::hours offlineAfter{1};    // Active -> Offline threshold (default: 1 hour)
    std::chrono::hours staleAfter{24};     // Offline -> Stale threshold (default: 24 hours / 1 day)
    std::chrono::hours archiveAfter{168};  // Stale -> Archived threshold (default: 168 hours / 7 days)
    std::chrono::hours purgeAfter{720};    // Archived -> Soft Deleted threshold (default: 720 hours / 30 days)
};

} // namespace NetDiscovery
