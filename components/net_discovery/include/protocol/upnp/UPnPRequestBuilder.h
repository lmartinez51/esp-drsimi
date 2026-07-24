/**
 * @file UPnPRequestBuilder.h
 * @brief Constructs immutable UPnPRequest value objects from translations and device context (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include "protocol/upnp/UPnPRequest.h"
#include "protocol/upnp/UPnPActionTranslator.h"
#include "protocol/upnp/UPnPDeviceContext.h"
#include "protocol/upnp/UPnPSoapSerializer.h"

#include <string>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Builder assembling UPnPRequest instances.
 *
 * Consumes translated semantic parameters, service metadata, and UPnPSoapSerializer.
 * Performs ZERO HTTP client operations, socket calls, or network communication.
 */
class UPnPRequestBuilder {
public:
    explicit UPnPRequestBuilder(UPnPSoapSerializer serializer = UPnPSoapSerializer());

    /**
     * @brief Assembles a complete, immutable UPnPRequest.
     */
    UPnPRequest BuildRequest(const UPnPActionTranslation&  translation,
                             const UPnPServiceDescriptor&  service,
                             const std::string&            baseUrl = "",
                             uint32_t                      timeoutMs = 5000) const;

private:
    UPnPSoapSerializer m_serializer;
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
