/**
 * @file DeviceFusionEngine.h
 * @brief Merges protocol endpoints into logical devices.
 */

#pragma once

#include "core/LogicalDevice.h"
#include "core/evidence/IdentityEvidence.h"
#include "core/IdentityScoringPolicy.h"

#include <vector>

namespace NetDiscovery {

/**
 * @brief Engine responsible for determining which protocol endpoints belong to the same physical device.
 */
class DeviceFusionEngine {
public:
    /**
     * @brief Internal strategy that evaluates evidence and clusters it into LogicalDevices.
     * @param evidences Raw evidence items.
     * @param policy Scoring weights for similarity matching.
     * @return A list of clustered LogicalDevices (unnormalized).
     */
    static std::vector<LogicalDevice> Fuse(const std::vector<IdentityEvidence>& evidences, const IdentityScoringPolicy& policy);
};

} // namespace NetDiscovery
