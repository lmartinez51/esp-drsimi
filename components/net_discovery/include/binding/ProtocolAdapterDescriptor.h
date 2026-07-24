/**
 * @file ProtocolAdapterDescriptor.h
 * @brief Pure immutable descriptor metadata model for Protocol Adapters (v5.0.0 Architecture Phase 8.1).
 * 
 * Represents the static, immutable architectural metadata describing a registered protocol adapter.
 * Contains ZERO mutable runtime execution state (which is owned exclusively by AdapterRuntimeState).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Binding {

using AdapterId = std::string;

/**
 * @brief Adapter operational health states (Enum definition used across binding models).
 */
enum class AdapterHealthState {
    Healthy,  // Fully functional without errors
    Degraded, // Operational with non-fatal degradation or elevated latency
    Offline,  // Unreachable or transport connection dropped
    Unknown   // Uninitialized state
};

/**
 * @brief Adapter runtime availability states (Enum definition used across binding models).
 */
enum class AdapterAvailability {
    Available,   // Ready to process execution requests
    Busy,        // Currently processing max concurrent requests
    Unavailable  // Temporarily locked or disabled
};

/**
 * @brief String conversion overload for AdapterHealthState.
 */
inline std::string ToString(AdapterHealthState health) {
    switch (health) {
        case AdapterHealthState::Healthy:  return "Healthy";
        case AdapterHealthState::Degraded: return "Degraded";
        case AdapterHealthState::Offline:  return "Offline";
        case AdapterHealthState::Unknown:
        default:                            return "Unknown";
    }
}

/**
 * @brief String conversion overload for AdapterAvailability.
 */
inline std::string ToString(AdapterAvailability availability) {
    switch (availability) {
        case AdapterAvailability::Available:   return "Available";
        case AdapterAvailability::Busy:        return "Busy";
        case AdapterAvailability::Unavailable: return "Unavailable";
        default:                               return "Unknown";
    }
}

/**
 * @brief Pure immutable descriptor describing a protocol adapter's static architectural properties.
 */
struct ProtocolAdapterDescriptor {
    AdapterId adapterId;                                    // Unique adapter identifier (e.g. "adapter.upnp.avtransport")
    std::string protocolName;                               // Protocol name (e.g. "UPnP", "Matter", "BLE")
    std::string transport;                                  // Underlying transport (e.g. "SOAP", "CoAP", "GATT")
    uint32_t version{1};                                    // Descriptor schema/metadata version
    std::vector<std::string> supportedOperations;           // Supported OperationId strings
    std::vector<std::string> supportedCapabilities;         // Supported CapabilityId strings
    std::unordered_map<std::string, std::string> metadata; // Extensible architectural metadata

    ProtocolAdapterDescriptor() = default;
    
    ProtocolAdapterDescriptor(AdapterId id, std::string protocol, std::string transportType, uint32_t ver = 1)
        : adapterId(std::move(id)), protocolName(std::move(protocol)), transport(std::move(transportType)), version(ver) {}

    bool operator==(const ProtocolAdapterDescriptor& other) const {
        return adapterId == other.adapterId && version == other.version;
    }
};

} // namespace Binding
} // namespace NetDiscovery
