/**
 * @file CapabilityResolver.h
 * @brief Resolves abstract capabilities based on NormalizedServices.
 */

#pragma once

#include "core/LogicalDevice.h"

namespace NetDiscovery {

/**
 * @brief Evaluates normalized services to deduce supported capabilities.
 */
class CapabilityResolver {
public:
    static void Resolve(LogicalDevice& device);
};

} // namespace NetDiscovery
