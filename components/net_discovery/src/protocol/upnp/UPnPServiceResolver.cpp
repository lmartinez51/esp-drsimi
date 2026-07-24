/**
 * @file UPnPServiceResolver.cpp
 * @brief Implementation of UPnPServiceResolver (v5.0.0 Architecture Phase 11).
 */

#include "protocol/upnp/UPnPServiceResolver.h"

#include <algorithm>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

std::optional<UPnPServiceDescriptor> UPnPServiceResolver::ResolveService(
        const UPnPDeviceContext& device,
        const std::string&       operationId,
        const std::string&       serviceHint) const {

    if (!device.IsValid()) return std::nullopt;

    // 1. Direct hint lookup
    if (!serviceHint.empty()) {
        const UPnPServiceDescriptor* match = device.FindService(serviceHint);
        if (match) return *match;
    }

    // 2. Direct operationId lookup (if operationId is a serviceType)
    if (!operationId.empty()) {
        const UPnPServiceDescriptor* match = device.FindService(operationId);
        if (match) return *match;
    }

    // 3. Operation heuristic mappings
    std::string targetServiceType;
    if (operationId == "Play" || operationId == "Pause" || operationId == "Stop" ||
        operationId == "Next" || operationId == "Previous" || operationId == "SetAVTransportURI" ||
        operationId == "GetTransportInfo") {
        targetServiceType = "urn:schemas-upnp-org:service:AVTransport:1";
    } else if (operationId == "SetVolume" || operationId == "GetVolume" ||
               operationId == "SetMute" || operationId == "GetMute") {
        targetServiceType = "urn:schemas-upnp-org:service:RenderingControl:1";
    } else if (operationId == "GetProtocolInfo" || operationId == "PrepareForConnection") {
        targetServiceType = "urn:schemas-upnp-org:service:ConnectionManager:1";
    }

    if (!targetServiceType.empty()) {
        const UPnPServiceDescriptor* match = device.FindService(targetServiceType);
        if (match) return *match;
    }

    // 4. Fallback: if only one service exists, return it
    if (device.services.size() == 1) {
        return device.services.begin()->second;
    }

    return std::nullopt;
}

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
