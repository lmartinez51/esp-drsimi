/**
 * @file ControllerRegistry.h
 * @brief Manages the list of available device controllers.
 */

#pragma once

#include "IDeviceController.h"
#include <memory>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Registry of all available IDeviceController implementations.
 */
class ControllerRegistry {
public:
    ControllerRegistry();

    /**
     * @brief Get all registered controllers.
     */
    const std::vector<std::unique_ptr<IDeviceController>>& GetControllers() const;

private:
    std::vector<std::unique_ptr<IDeviceController>> m_controllers;
};

} // namespace NetDiscovery
