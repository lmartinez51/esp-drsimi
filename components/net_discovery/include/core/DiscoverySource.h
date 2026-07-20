/**
 * @file DiscoverySource.h
 * @brief Enum tracking the origin of device information.
 */

#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief How the device information was discovered.
 * A device may have multiple discovery sources.
 */
enum class DiscoverySource {
    SSDP,
    UPnP_XML,
    mDNS,
    BLE,
    Matter,
    Manual,
    Cached,
    Unknown
};

/**
 * @brief Convert DiscoverySource to a human-readable string.
 */
inline std::string ToString(DiscoverySource source) {
    switch (source) {
        case DiscoverySource::SSDP:     return "SSDP";
        case DiscoverySource::UPnP_XML: return "UPnP XML";
        case DiscoverySource::mDNS:     return "mDNS";
        case DiscoverySource::BLE:      return "BLE";
        case DiscoverySource::Matter:   return "Matter";
        case DiscoverySource::Manual:   return "Manual";
        case DiscoverySource::Cached:   return "Cached";
        default:                        return "Unknown";
    }
}

} // namespace NetDiscovery
