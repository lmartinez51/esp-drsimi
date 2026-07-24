/**
 * @file HTTPRequest.h
 * @brief Immutable value object representing an HTTP operation request (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief HTTP Method enum.
 */
enum class HTTPMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

/**
 * @brief Converts HTTPMethod enum to string representation.
 */
inline std::string ToString(HTTPMethod method) {
    switch (method) {
        case HTTPMethod::GET:    return "GET";
        case HTTPMethod::POST:   return "POST";
        case HTTPMethod::PUT:    return "PUT";
        case HTTPMethod::DELETE: return "DELETE";
        case HTTPMethod::PATCH:  return "PATCH";
        default:                 return "GET";
    }
}

/**
 * @brief Immutable request payload carrying method, URL, headers, query params, body, and timeout.
 *
 * Contains ZERO socket or parsing logic. Pure value object.
 */
struct HTTPRequest {
    HTTPMethod  method{HTTPMethod::GET};
    std::string url;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> queryParams;
    std::string body;
    uint32_t    timeoutMs{5000};
    std::unordered_map<std::string, std::string> metadata;

    HTTPRequest() = default;

    HTTPRequest(HTTPMethod m,
                std::string u,
                std::unordered_map<std::string, std::string> h = {},
                std::unordered_map<std::string, std::string> q = {},
                std::string b = "",
                uint32_t timeout = 5000)
        : method(m)
        , url(std::move(u))
        , headers(std::move(h))
        , queryParams(std::move(q))
        , body(std::move(b))
        , timeoutMs(timeout) {}

    bool IsValid() const { return !url.empty(); }
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
