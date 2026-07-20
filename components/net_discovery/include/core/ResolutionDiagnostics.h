/**
 * @file ResolutionDiagnostics.h
 * @brief Diagnostics emitted by a controller during evaluation.
 */

#pragma once

#include "Capability.h"
#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Detailed diagnostic output from a controller evaluation.
 */
struct ResolutionDiagnostics {
    int score{0};
    bool matchedManufacturer{false};
    bool matchedModel{false};
    std::vector<std::string> matchedFeatures;
    std::vector<Capability> matchedCapabilities;
    std::string reason;
    
    // Detailed breakdown of the score
    std::vector<std::pair<std::string, int>> scoreBreakdown;
};

} // namespace NetDiscovery
