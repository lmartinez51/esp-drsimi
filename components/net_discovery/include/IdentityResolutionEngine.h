/**
 * @file IdentityResolutionEngine.h
 * @brief Evaluates evidence to group protocol endpoints into physical LogicalDevices.
 */

#pragma once

#include "core/LogicalDevice.h"
#include "core/evidence/IdentityEvidence.h"
#include "core/IdentityScoringPolicy.h"
#include <vector>

namespace NetDiscovery {

/**
 * @brief Evaluates protocol evidence and clusters it into LogicalDevices.
 */
class IdentityResolutionEngine {
public:
    explicit IdentityResolutionEngine(IdentityScoringPolicy policy = IdentityScoringPolicy{});

    /**
     * @brief Evaluates all provided evidence and fuses them into LogicalDevices.
     * @param evidences Raw evidence items from various discovery providers.
     * @return A list of completely normalized LogicalDevices with DeviceSignatures.
     */
    std::vector<LogicalDevice> Resolve(const std::vector<IdentityEvidence>& evidences) const;

private:
    IdentityScoringPolicy m_policy;
    
    // Evaluates identity confidence and populates the breakdown and overall score
    void ComputeIdentityConfidence(LogicalDevice& device) const;
    
    // Normalizes collected endpoints into a single DeviceSignature
    void BuildDeviceSignature(LogicalDevice& device) const;
};

} // namespace NetDiscovery
