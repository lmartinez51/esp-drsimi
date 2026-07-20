/**
 * @file ControllerCandidate.h
 * @brief Representation of an evaluated controller candidate for a device.
 */

#pragma once

#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief A controller that was evaluated for a device.
 */
struct ControllerCandidate {
    std::string name;
    int confidence{0};
    std::string diagnosticReason;
    bool isRejected{false};
    
    // Detailed list of score contributions for explicability
    std::vector<std::pair<std::string, int>> scoreBreakdown;
};

} // namespace NetDiscovery
