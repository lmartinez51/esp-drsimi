/**
 * @file UPnPResponse.h
 * @brief Immutable value object representing a UPnP HTTP/SOAP response (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include "protocol/upnp/UPnPTransportDiagnostics.h"

#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Immutable response payload carrying HTTP status, headers, and raw XML body.
 *
 * Contains ZERO parsing logic. Pure value object.
 */
struct UPnPResponse {
    int32_t                  statusCode{0};
    std::string              statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string              xmlPayload;
    uint32_t                 elapsedTimeMs{0};
    UPnPTransportDiagnostics diagnostics;

    UPnPResponse() = default;

    UPnPResponse(int32_t code,
                 std::string text,
                 std::string payload,
                 uint32_t elapsed = 0,
                 std::unordered_map<std::string, std::string> hdrs = {},
                 UPnPTransportDiagnostics diag = {})
        : statusCode(code)
        , statusText(std::move(text))
        , headers(std::move(hdrs))
        , xmlPayload(std::move(payload))
        , elapsedTimeMs(elapsed)
        , diagnostics(diag) {}

    bool IsSuccess() const { return statusCode >= 200 && statusCode < 300; }
    bool IsTimeout() const { return statusCode == 0 || statusCode == 408; }
    bool IsHttpError() const { return statusCode >= 400; }
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
