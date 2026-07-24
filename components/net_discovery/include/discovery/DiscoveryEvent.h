/**
 * @file DiscoveryEvent.h
 * @brief Immutable discovery lifecycle events (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/DiscoveredDeviceDescriptor.h"
#include <string>
#include <utility>

namespace NetDiscovery {
namespace Discovery {

enum class DiscoveryEventType {
    DeviceDiscovered,
    DeviceUpdated,
    DeviceLost,
    DiscoveryCompleted
};

inline std::string ToString(DiscoveryEventType type) {
    switch (type) {
        case DiscoveryEventType::DeviceDiscovered:   return "DeviceDiscovered";
        case DiscoveryEventType::DeviceUpdated:      return "DeviceUpdated";
        case DiscoveryEventType::DeviceLost:         return "DeviceLost";
        case DiscoveryEventType::DiscoveryCompleted: return "DiscoveryCompleted";
        default:                                     return "Unknown";
    }
}

/**
 * @brief Immutable event carrying discovery state changes.
 */
struct DiscoveryEvent {
    DiscoveryEventType         eventType{DiscoveryEventType::DeviceDiscovered};
    std::string                providerId;
    DiscoveredDeviceDescriptor device;
    uint64_t                   timestampMs{0};

    DiscoveryEvent() = default;

    DiscoveryEvent(DiscoveryEventType type, std::string pId, DiscoveredDeviceDescriptor dev, uint64_t ts = 0)
        : eventType(type), providerId(std::move(pId)), device(std::move(dev)), timestampMs(ts) {}
};

} // namespace Discovery
} // namespace NetDiscovery
