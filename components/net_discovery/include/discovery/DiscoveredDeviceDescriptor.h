/**
 * @file DiscoveredDeviceDescriptor.h
 * @brief Immutable unified device representation bridging physical discovery and semantic execution (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

namespace NetDiscovery {
namespace Discovery {

/**
 * @brief Immutable unified device descriptor.
 *
 * Exposes device metadata strictly through semantic and abstract protocol capabilities.
 * Raw IP addresses, SOAP control URLs, and SSDP headers are encapsulated inside adapterHints/metadata.
 */
struct DiscoveredDeviceDescriptor {
    std::string deviceId;
    std::string friendlyName;
    std::string manufacturer;
    std::string model;
    std::string room;
    std::vector<std::string> networkAddresses;
    std::vector<std::string> supportedProtocols;     ///< e.g. "UPnP", "mDNS", "BLE", "HTTP", "MQTT"
    std::vector<std::string> semanticCapabilities;   ///< e.g. "PowerControl", "Dimmer", "AudioPlayback"
    std::unordered_map<std::string, std::string> adapterHints;
    std::unordered_map<std::string, std::string> metadata;

    DiscoveredDeviceDescriptor() = default;

    DiscoveredDeviceDescriptor(std::string id,
                               std::string name,
                               std::string mfr = "",
                               std::string mdl = "",
                               std::string rm = "",
                               std::vector<std::string> addrs = {},
                               std::vector<std::string> protocols = {},
                               std::vector<std::string> caps = {},
                               std::unordered_map<std::string, std::string> hints = {},
                               std::unordered_map<std::string, std::string> meta = {})
        : deviceId(std::move(id))
        , friendlyName(std::move(name))
        , manufacturer(std::move(mfr))
        , model(std::move(mdl))
        , room(std::move(rm))
        , networkAddresses(std::move(addrs))
        , supportedProtocols(std::move(protocols))
        , semanticCapabilities(std::move(caps))
        , adapterHints(std::move(hints))
        , metadata(std::move(meta)) {}

    bool IsValid() const { return !deviceId.empty(); }

    bool HasProtocol(const std::string& protocol) const {
        for (const auto& p : supportedProtocols) {
            if (p == protocol) return true;
        }
        return false;
    }

    bool HasCapability(const std::string& capability) const {
        for (const auto& c : semanticCapabilities) {
            if (c == capability) return true;
        }
        return false;
    }
};

} // namespace Discovery
} // namespace NetDiscovery
