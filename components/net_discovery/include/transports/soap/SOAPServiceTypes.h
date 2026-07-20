#pragma once

#include <string>

namespace netdiscovery {
namespace soap {

/**
 * @brief Strongly typed service identifiers for UPnP SOAP services.
 * 
 * Replaces string-based URN comparisons (e.g., "urn:schemas-upnp-org:service:RenderingControl:1")
 * to improve type safety and performance when routing responses to the correct parser.
 */
enum class UPnPService {
    RenderingControl,
    AVTransport,
    ConnectionManager,
    Unknown
};

/**
 * @brief Helper function to convert a raw UPnP service URN to a strongly typed enum.
 */
inline UPnPService ParseServiceType(const std::string& serviceTypeURN) {
    if (serviceTypeURN.find("RenderingControl") != std::string::npos) {
        return UPnPService::RenderingControl;
    }
    if (serviceTypeURN.find("AVTransport") != std::string::npos) {
        return UPnPService::AVTransport;
    }
    if (serviceTypeURN.find("ConnectionManager") != std::string::npos) {
        return UPnPService::ConnectionManager;
    }
    return UPnPService::Unknown;
}

} // namespace soap
} // namespace netdiscovery
