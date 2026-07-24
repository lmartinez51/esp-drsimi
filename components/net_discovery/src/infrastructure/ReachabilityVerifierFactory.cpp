/**
 * @file ReachabilityVerifierFactory.cpp
 * @brief Implementation of ReachabilityVerifierFactory (v5.1.0 Phase B).
 */

#include "infrastructure/ReachabilityVerifierFactory.h"
#include "infrastructure/DefaultReachabilityVerifier.h"

namespace NetDiscovery {

std::shared_ptr<IReachabilityVerifier> ReachabilityVerifierFactory::CreateVerifier(const LogicalDevice& device) {
    // Returns default verifier, extension point for TCP/HTTP/SSDP/BLE/Matter verifiers
    return std::make_shared<DefaultReachabilityVerifier>();
}

} // namespace NetDiscovery
