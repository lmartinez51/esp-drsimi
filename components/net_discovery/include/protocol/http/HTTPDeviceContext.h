/**
 * @file HTTPDeviceContext.h
 * @brief Immutable metadata container for HTTP target devices (v5.0.0 Architecture Phase 15).
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
 * @brief Immutable metadata container for an HTTP REST target endpoint/device.
 *
 * Contains ZERO mutable runtime session variables.
 */
struct HTTPDeviceContext {
    std::string baseUrl;                                    ///< e.g., "http://192.168.1.100"
    std::string hostname;                                   ///< e.g., "192.168.1.100" or "api.shelly.cloud"
    uint16_t    port{80};                                   ///< Default HTTP port
    bool        useTls{false};                              ///< HTTPS flag
    std::string authType{"None"};                           ///< "None", "Basic", "Bearer", "APIKey"
    std::unordered_map<std::string, std::string> defaultHeaders;
    std::unordered_map<std::string, std::string> metadata;

    HTTPDeviceContext() = default;

    HTTPDeviceContext(std::string url,
                      std::string host,
                      uint16_t p = 80,
                      bool tls = false,
                      std::string auth = "None")
        : baseUrl(std::move(url))
        , hostname(std::move(host))
        , port(p)
        , useTls(tls)
        , authType(std::move(auth)) {}

    bool IsValid() const { return !baseUrl.empty() || !hostname.empty(); }
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
