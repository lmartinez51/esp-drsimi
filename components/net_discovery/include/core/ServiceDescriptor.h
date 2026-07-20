#pragma once

#include "StandardService.h"
#include <string>

namespace NetDiscovery {

/**
 * @brief A generic representation of an exposed service on a LogicalDevice.
 */
struct ServiceDescriptor {
    std::string uniqueId;       // e.g., rootUuid + "::" + serviceId
    std::string protocolFamily; // e.g., "UPnP", "DIAL"
    
    StandardService standardType{StandardService::Unknown};
    
    std::string serviceType;    // The raw string type (e.g., "urn:schemas-upnp-org:service:RenderingControl:1")
    std::string serviceId;      // The raw ID
    
    std::string controlUrl;
    std::string eventUrl;
    std::string scpdUrl;
};

} // namespace NetDiscovery
