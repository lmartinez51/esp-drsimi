/**
 * @file UPnPRequestBuilder.cpp
 * @brief Implementation of UPnPRequestBuilder (v5.0.0 Architecture Phase 11.1).
 */

#include "protocol/upnp/UPnPRequestBuilder.h"

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

UPnPRequestBuilder::UPnPRequestBuilder(UPnPSoapSerializer serializer)
    : m_serializer(std::move(serializer)) {}

UPnPRequest UPnPRequestBuilder::BuildRequest(
        const UPnPActionTranslation& translation,
        const UPnPServiceDescriptor& service,
        const std::string& baseUrl,
        uint32_t timeoutMs) const {

    UPnPRequest request;

    // Resolve full control URL
    std::string fullUrl = service.controlUrl;
    if (!baseUrl.empty() && fullUrl.find("http://") != 0 && fullUrl.find("https://") != 0) {
        if (baseUrl.back() == '/' && !fullUrl.empty() && fullUrl.front() == '/') {
            fullUrl = baseUrl + fullUrl.substr(1);
        } else if (baseUrl.back() != '/' && !fullUrl.empty() && fullUrl.front() != '/') {
            fullUrl = baseUrl + "/" + fullUrl;
        } else {
            fullUrl = baseUrl + fullUrl;
        }
    }
    request.controlUrl = fullUrl;

    request.soapAction  = "\"" + service.serviceType + "#" + translation.operationName + "\"";
    request.xmlBody     = m_serializer.SerializeEnvelope(service.serviceType, translation.operationName, translation.arguments);
    request.timeoutMs   = timeoutMs > 0 ? timeoutMs : 5000;
    request.httpMethod  = "POST";

    request.headers["SOAPACTION"]   = request.soapAction;
    request.headers["Content-Type"] = "text/xml; charset=\"utf-8\"";

    return request;
}

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
