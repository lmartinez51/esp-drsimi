#pragma once
#include <string>
#include <map>

namespace NetDiscovery {

/**
 * @brief An immutable representation of a completely formulated SOAP HTTP Request.
 * Ready to be executed by SOAPTransport.
 */
struct SOAPRequest {
    std::string endpointUrl;
    std::string soapAction;
    std::string contentType;
    std::string xmlBody;
    int timeoutMs = 5000;
    std::map<std::string, std::string> extraHeaders;
};

} // namespace NetDiscovery
