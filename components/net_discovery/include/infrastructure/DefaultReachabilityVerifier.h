/**
 * @file DefaultReachabilityVerifier.h
 * @brief Default reachability verifier checking endpoint IP availability (v5.1.0 Phase B).
 */

#pragma once

#include "infrastructure/IReachabilityVerifier.h"

namespace NetDiscovery {

class DefaultReachabilityVerifier : public IReachabilityVerifier {
public:
    DefaultReachabilityVerifier() = default;

    bool Verify(const LogicalDevice& device, const ExecutionRoute* route, std::chrono::milliseconds timeoutMs) override;
};

} // namespace NetDiscovery
