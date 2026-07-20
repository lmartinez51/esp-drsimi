/**
 * @file ControllerResolver.h
 * @brief Evaluates controllers for a given device.
 */

#pragma once

#include "core/LogicalDevice.h"
#include "ControllerRegistry.h"

namespace NetDiscovery {

/**
 * @brief Resolves a LogicalDevice into the best controller instance and populates its actions and capabilities.
 */
class ControllerResolver {
public:
    ControllerResolver(const ControllerRegistry& registry);

    /**
     * @brief Resolve the best controller for a logical device based on its fingerprint,
     *        and populates the device's controllers, capabilities, and actions.
     * @param device The LogicalDevice to resolve and mutate.
     */
    void Resolve(LogicalDevice& device) const;

private:
    const ControllerRegistry& m_registry;
};

} // namespace NetDiscovery
