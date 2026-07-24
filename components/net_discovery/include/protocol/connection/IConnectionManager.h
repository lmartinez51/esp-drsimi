/**
 * @file IConnectionManager.h
 * @brief Pure abstract interface for protocol-independent connection management (v5.0.0 Architecture Phase 11.2).
 */

#pragma once

#include "protocol/connection/ConnectionHandle.h"
#include "protocol/connection/ConnectionState.h"
#include "protocol/connection/ConnectionStatistics.h"

#include <optional>
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Pure abstract interface for connection management shared by all protocol adapters.
 *
 * Totally protocol-independent. No HTTP, BLE, MQTT, Matter, or socket details.
 */
class IConnectionManager {
public:
    virtual ~IConnectionManager() = default;

    /**
     * @brief Leases or creates a ConnectionHandle for a target protocol and endpoint.
     */
    virtual std::optional<ConnectionHandle> AcquireConnection(
        const std::string& protocol,
        const std::string& endpoint,
        uint32_t timeoutMs = 5000) = 0;

    /**
     * @brief Releases a leased connection back to the pool.
     */
    virtual void ReleaseConnection(const ConnectionHandle& handle) = 0;

    /**
     * @brief Validates if a leased connection handle is still active and valid.
     */
    virtual bool ValidateConnection(const ConnectionHandle& handle) = 0;

    /**
     * @brief Forces immediate closure of a connection by connectionId.
     */
    virtual void CloseConnection(const ConnectionId& connectionId) = 0;

    /**
     * @brief Closes idle connections exceeding maxIdleTimeMs.
     * @return Number of connections closed.
     */
    virtual uint32_t CloseIdleConnections(uint32_t maxIdleTimeMs) = 0;

    /**
     * @brief Closes all active and idle connections and resets pool state.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Returns telemetry statistics.
     */
    virtual const ConnectionStatistics& GetStatistics() const = 0;
};

} // namespace Protocol
} // namespace NetDiscovery
