/**
 * @file UPnPServiceResolver.h
 * @brief Resolves service descriptors from discovered device context metadata (v5.0.0 Architecture Phase 11).
 */

#pragma once

#include "protocol/upnp/UPnPDeviceContext.h"

#include <optional>
#include <string>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Resolves UPnPServiceDescriptor from previously discovered device metadata.
 *
 * Must NEVER perform network communication or socket queries. Uses exclusively
 * the metadata stored in UPnPDeviceContext.
 */
class UPnPServiceResolver {
public:
    UPnPServiceResolver() = default;

    /**
     * @brief Resolves a service descriptor for a given operationId or service hint.
     *
     * Rules:
     *   1. Look up exact match by serviceType or serviceId in device.services.
     *   2. Look up operation mapping heuristics (e.g. "SetVolume" -> RenderingControl, "Play" -> AVTransport).
     *   3. Return std::nullopt if no matching service descriptor exists.
     */
    std::optional<UPnPServiceDescriptor> ResolveService(
        const UPnPDeviceContext& device,
        const std::string&       operationId,
        const std::string&       serviceHint = "") const;
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
