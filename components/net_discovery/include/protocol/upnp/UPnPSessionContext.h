/**
 * @file UPnPSessionContext.h
 * @brief Mutable runtime session state for UPnP communications (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Mutable runtime state for a UPnP adapter communication session.
 *
 * Owned per device connection. Strictly separated from immutable UPnPDeviceContext.
 */
struct UPnPSessionContext {
    std::string udn;                    ///< Target device UDN
    uint32_t    sequenceNumber{0};      ///< Event subscription / request sequence counter
    std::string authHeader;             ///< HTTP Authorization token if required
    std::string cookieHeader;           ///< Session cookies
    bool        isConnected{true};      ///< Dynamic connection reachability status
    bool        keepAliveActive{false}; ///< Keep-alive connection flag
    std::string lastError;              ///< Human-readable text of last error
    uint32_t    lastLatencyMs{0};       ///< Latency of last successful request (ms)
    uint32_t    retryCount{0};          ///< Transient retry counter for current session
    uint32_t    reconnectCount{0};      ///< Reconnection count
    uint64_t    lastActivityTimestampMs{0}; ///< Timestamp of last network activity
    uint64_t    stateVersion{0};        ///< Version counter incremented on state mutations
    std::unordered_map<std::string, std::string> sessionVariables;

    UPnPSessionContext() = default;
    explicit UPnPSessionContext(std::string deviceUdn)
        : udn(std::move(deviceUdn)) {}

    void NextSequence() { ++sequenceNumber; }
    void BumpVersion() { ++stateVersion; }
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
