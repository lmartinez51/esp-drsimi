/**
 * @file BindingSelectionResult.h
 * @brief Selection result and detailed score breakdown object (v5.0.0 Architecture Phase 8).
 * 
 * Returned by BindingSelector to provide full transparency, diagnostic scores, human-readable 
 * and AI-explainable rationale, and prioritized fallback options.
 */

#pragma once

#include "binding/ActionBinding.h"

#include <optional>
#include <string>
#include <vector>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Multi-dimensional scoring breakdown for a candidate ActionBinding.
 */
struct BindingScore {
    int priorityScore{0};       // Score derived from protocol priority (e.g. Matter=100, UPnP=90)
    int healthScore{0};         // Score derived from adapter health state (+50 Healthy, +20 Degraded)
    int availabilityScore{0};   // Score derived from adapter availability (+50 Available, 0 Busy)
    int authenticationScore{0}; // Score derived from auth readiness (+10 ready, -30 unauthenticated)
    int totalScore{0};          // Aggregated total match score used for deterministic ranking
};

/**
 * @brief Complete result of a BindingSelector query containing the optimal binding and fallback options.
 */
struct BindingSelectionResult {
    std::optional<ActionBinding> selectedBinding; // Primary chosen ActionBinding (std::nullopt if no valid candidate)
    BindingScore score;                            // Detailed match score breakdown
    std::string selectionReason;                   // Explainable selection or rejection rationale
    std::vector<ActionBinding> fallbackCandidates; // Ordered list of backup ActionBindings if primary fails

    bool HasSelection() const { return selectedBinding.has_value(); }
};

} // namespace Binding
} // namespace NetDiscovery
