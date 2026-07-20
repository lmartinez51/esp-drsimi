/**
 * @file NormalizedService.h
 * @brief Representation of a service independent of the discovery protocol.
 */

#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief Protocol-neutral representation of a service or endpoint functionality.
 */
struct NormalizedService {
    std::string name;       // e.g., "RenderingControl", "DIAL", "RemoteControlReceiver"
    std::string domain;     // e.g., "upnp", "dial", "samsung", "matter"
    std::string version;    // e.g., "1"
    
    // Optional protocol-specific opaque data, useful if a controller needs to know the exact endpoint path later,
    // but the classifier and capability resolver only care about name and domain.
    std::string endpointUrl; 
};

} // namespace NetDiscovery
