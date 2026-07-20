/**
 * @file DeviceClassifier.h
 * @brief Classifies a logical device into a primary class and device roles.
 */

#pragma once

#include "core/LogicalDevice.h"

namespace NetDiscovery {

/**
 * @brief Explicitly determines the form factor (PrimaryDeviceClass) and roles (DeviceRole) of a LogicalDevice.
 */
class DeviceClassifier {
public:
    static void Classify(LogicalDevice& device);
};

} // namespace NetDiscovery
