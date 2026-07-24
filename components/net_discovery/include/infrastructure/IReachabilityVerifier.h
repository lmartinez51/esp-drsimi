/**
 * @file IReachabilityVerifier.h
 * @brief Pure interface for protocol-specific reachability verifiers (v5.1.0 Phase B).
 */

#pragma once

#include "core/LogicalDevice.h"
#include "core/ExecutionRoute.h"
#include <chrono>

namespace NetDiscovery {

/**
 * @brief Abstract interface for verifying device reachability prior to execution.
 */
class IReachabilityVerifier {
public:
    virtual ~IReachabilityVerifier() = default;

    virtual bool Verify(const LogicalDevice& device, const ExecutionRoute* route, std::chrono::milliseconds timeoutMs) = 0;
};

} // namespace NetDiscovery
