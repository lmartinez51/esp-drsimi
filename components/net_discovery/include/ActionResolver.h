/**
 * @file ActionResolver.h
 * @brief Resolves generic capabilities into concrete executable actions.
 */

#pragma once

#include "core/LogicalDevice.h"

namespace NetDiscovery {

/**
 * @brief Expands high-level capabilities into standard actions.
 */
class ActionResolver {
public:
    static void Resolve(LogicalDevice& device);
};

} // namespace NetDiscovery
