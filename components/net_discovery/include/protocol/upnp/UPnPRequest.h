/**
 * @file UPnPRequest.h
 * @brief Immutable value object representing a UPnP HTTP/SOAP request (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Immutable request payload carrying target URL, headers, and XML body.
 *
 * Contains ZERO parsing or networking logic. Pure value object.
 */
struct UPnPRequest {
    std::string httpMethod{"POST"};
    std::string controlUrl;
    std::string soapAction;
    std::unordered_map<std::string, std::string> headers;
    std::string xmlBody;
    uint32_t    timeoutMs{5000};
    std::unordered_map<std::string, std::string> metadata;

    UPnPRequest() = default;

    UPnPRequest(std::string url,
                std::string action,
                std::string body,
                uint32_t timeout = 5000,
                std::unordered_map<std::string, std::string> hdrs = {},
                std::string method = "POST")
        : httpMethod(std::move(method))
        , controlUrl(std::move(url))
        , soapAction(std::move(action))
        , headers(std::move(hdrs))
        , xmlBody(std::move(body))
        , timeoutMs(timeout) {}

    bool IsValid() const { return !controlUrl.empty() && !soapAction.empty(); }
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
