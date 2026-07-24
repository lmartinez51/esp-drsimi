/**
 * @file UPnPDeviceContext.h
 * @brief Immutable metadata for a discovered UPnP device (v5.0.0 Architecture Phase 11).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Metadata descriptor for a single UPnP service discovered on a device.
 */
struct UPnPServiceDescriptor {
    std::string serviceType;    ///< e.g., "urn:schemas-upnp-org:service:AVTransport:1"
    std::string serviceId;      ///< e.g., "urn:upnp-org:serviceId:AVTransport"
    std::string controlUrl;     ///< Relative or absolute URL for control POSTs
    std::string eventSubUrl;    ///< Relative or absolute URL for event subscriptions
    std::string scpdUrl;        ///< Relative or absolute URL for SCPD XML description
};

/**
 * @brief Immutable metadata container for one discovered UPnP physical device.
 *
 * Contains ZERO mutable runtime variables (sequence numbers, tokens, connections).
 * Mutable runtime variables are strictly carried by UPnPSessionContext.
 */
struct UPnPDeviceContext {
    std::string udn;            ///< Unique Device Name (UUID)
    std::string friendlyName;   ///< User-visible device name
    std::string manufacturer;   ///< Device manufacturer
    std::string modelName;      ///< Device model name
    std::string deviceType;     ///< e.g., "urn:schemas-upnp-org:device:MediaRenderer:1"
    std::string baseUrl;        ///< Base URL for relative control path resolution

    /// Discovered services mapped by serviceType or serviceId
    std::unordered_map<std::string, UPnPServiceDescriptor> services;

    /// Capability tags (e.g., "av_transport", "rendering_control", "connection_manager")
    std::vector<std::string> capabilities;

    /// Extensible metadata
    std::unordered_map<std::string, std::string> metadata;

    UPnPDeviceContext() = default;

    UPnPDeviceContext(std::string deviceUdn,
                      std::string name,
                      std::string mfr = "",
                      std::string model = "",
                      std::string type = "",
                      std::string base = "")
        : udn(std::move(deviceUdn))
        , friendlyName(std::move(name))
        , manufacturer(std::move(mfr))
        , modelName(std::move(model))
        , deviceType(std::move(type))
        , baseUrl(std::move(base)) {}

    bool IsValid() const { return !udn.empty(); }

    const UPnPServiceDescriptor* FindService(const std::string& key) const {
        auto it = services.find(key);
        if (it != services.end()) return &it->second;
        for (const auto& kv : services) {
            if (kv.second.serviceType == key || kv.second.serviceId == key) {
                return &kv.second;
            }
        }
        return nullptr;
    }
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
