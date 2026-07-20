/**
 * @file ProtocolNormalizer.h
 * @brief Normalizes protocol-specific evidence into generic NormalizedServices.
 */

#pragma once

#include "core/LogicalDevice.h"

namespace NetDiscovery {

/**
 * @brief Translates protocol-specific concepts into NormalizedServices.
 */
class ProtocolNormalizer {
public:
    /**
     * @brief Extracts generic services from all endpoints in a LogicalDevice.
     * @param device The device to normalize. Its normalizedServices list will be populated.
     */
    static void Normalize(LogicalDevice& device);
};

} // namespace NetDiscovery
