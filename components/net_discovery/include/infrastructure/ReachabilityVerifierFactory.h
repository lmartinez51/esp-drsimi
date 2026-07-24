/**
 * @file ReachabilityVerifierFactory.h
 * @brief Factory producing protocol-specific IReachabilityVerifier instances (v5.1.0 Phase B).
 */

#pragma once

#include "infrastructure/IReachabilityVerifier.h"
#include <memory>

namespace NetDiscovery {

class ReachabilityVerifierFactory {
public:
    static std::shared_ptr<IReachabilityVerifier> CreateVerifier(const LogicalDevice& device);
};

} // namespace NetDiscovery
