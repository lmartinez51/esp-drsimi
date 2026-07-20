/**
 * @file ControllerRegistry.cpp
 * @brief ControllerRegistry implementation.
 */

#include "../include/ControllerRegistry.h"
#include "../include/controllers/GenericDLNAController.h"
#include "../include/controllers/SamsungController.h"
#include "../include/controllers/UnknownController.h"

namespace NetDiscovery {

ControllerRegistry::ControllerRegistry() {
    m_controllers.push_back(std::make_unique<SamsungController>());
    m_controllers.push_back(std::make_unique<GenericDLNAController>());
    m_controllers.push_back(std::make_unique<UnknownController>());
}

const std::vector<std::unique_ptr<IDeviceController>>& ControllerRegistry::GetControllers() const {
    return m_controllers;
}

} // namespace NetDiscovery
