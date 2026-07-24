/**
 * @file HTTPSessionContext.h
 * @brief Mutable runtime state for HTTP sessions (v5.0.0 Architecture Phase 15).
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
 * @brief Mutable runtime state for an HTTP session.
 *
 * Strictly separated from immutable HTTPDeviceContext.
 */
struct HTTPSessionContext {
    std::string sessionId;
    std::string authenticationState{"Authenticated"};
    uint64_t    lastActivityTimestampMs{0};
    uint64_t    stateVersion{0};
    std::unordered_map<std::string, std::string> sessionVariables;

    HTTPSessionContext() = default;
    explicit HTTPSessionContext(std::string id)
        : sessionId(std::move(id)) {}

    void BumpVersion() { ++stateVersion; }
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
