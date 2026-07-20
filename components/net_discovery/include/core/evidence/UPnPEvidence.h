/**
 * @file UPnPEvidence.h
 * @brief Structured evidence collected via UPnP.
 */

#pragma once

#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief A single UPnP service advertised by a device.
 */
struct UPnPService {
    std::string serviceType;
    std::string serviceId;
    std::string controlUrl;
    std::string eventUrl;
    std::string scpdUrl;
};

/**
 * @brief Representation of an icon advertised by a UPnP device.
 */
struct UPnPIcon {
    std::string mimetype;
    int width{0};
    int height{0};
    int depth{0};
    std::string url;
};

/**
 * @brief Protocol-specific evidence collected from UPnP Device Descriptions.
 */
struct UPnPEvidence {
    std::string rawXml;
    std::string locationUrl;
    std::string applicationUrl;
    std::vector<UPnPService> services;
    std::vector<UPnPIcon> icons;
    std::string deviceType;
    std::string parentUuid;
    std::vector<std::string> embeddedDeviceUuids;
};

} // namespace NetDiscovery
