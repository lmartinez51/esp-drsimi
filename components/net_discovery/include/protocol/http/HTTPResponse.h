/**
 * @file HTTPResponse.h
 * @brief Immutable value object representing an HTTP operation response (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Immutable response payload carrying status code, headers, payload body, latency, and bytes.
 *
 * Contains ZERO socket or parser logic. Pure value object.
 */
struct HTTPResponse {
    int32_t     statusCode{0};              ///< e.g. 200, 201, 400, 404, 500, or -101 for timeout
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string payload;
    uint32_t    latencyMs{0};
    uint32_t    bytesSent{0};
    uint32_t    bytesReceived{0};
    std::vector<std::string> diagnostics;
    std::unordered_map<std::string, std::string> metadata;

    HTTPResponse() = default;

    HTTPResponse(int32_t code,
                 std::string text,
                 std::unordered_map<std::string, std::string> h = {},
                 std::string body = "",
                 uint32_t latency = 0,
                 uint32_t txBytes = 0,
                 uint32_t rxBytes = 0)
        : statusCode(code)
        , statusText(std::move(text))
        , headers(std::move(h))
        , payload(std::move(body))
        , latencyMs(latency)
        , bytesSent(txBytes)
        , bytesReceived(rxBytes) {}

    bool IsSuccess() const { return statusCode >= 200 && statusCode < 300; }
    bool IsTimeout() const { return statusCode == -101; }
    bool IsClientError() const { return statusCode >= 400 && statusCode < 500; }
    bool IsServerError() const { return statusCode >= 500 && statusCode < 600; }
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
