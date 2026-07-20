/**
 * @file DeviceSignature.h
 * @brief Completely normalized identity representation, independent of protocol.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include "Capability.h"
#include "DiscoverySource.h"

namespace NetDiscovery {

/**
 * @brief A normalized identity signature for a physical device.
 * Used by IdentityResolutionEngine and ControllerResolver.
 */
struct DeviceSignature {
    std::string manufacturer;
    std::string model;
    std::string friendlyName;
    std::string serialNumber;
    std::string presentationUrl;
    
    std::vector<std::string> namespaces;
    std::vector<std::string> deviceTypes;
    std::vector<std::string> serviceTypes;
    std::vector<Capability> capabilities;
    std::vector<DiscoverySource> discoverySources;
    
    std::vector<std::string> ips;
    std::optional<std::string> mac; // Future-proof for when MAC is available

    // Helper to check if a specific namespace/feature is present
    bool HasFeature(const std::string& feature) const {
        for (const auto& ns : namespaces) {
            if (ns.find(feature) != std::string::npos) return true;
        }
        for (const auto& dt : deviceTypes) {
            if (dt.find(feature) != std::string::npos) return true;
        }
        for (const auto& st : serviceTypes) {
            if (st.find(feature) != std::string::npos) return true;
        }
        return false;
    }
};

} // namespace NetDiscovery
