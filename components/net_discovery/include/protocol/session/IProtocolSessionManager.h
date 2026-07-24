/**
 * @file IProtocolSessionManager.h
 * @brief Pure abstract interface for protocol-independent session management (v5.0.0 Architecture Phase 12.1).
 */

#pragma once

#include "protocol/session/ProtocolSession.h"
#include "protocol/session/ProtocolSessionContext.h"
#include "protocol/session/ProtocolSessionStatistics.h"

#include <optional>
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Pure abstract interface for protocol session management shared by all protocol adapters.
 *
 * Totally protocol-independent. Contains ZERO protocol-specific, network, or socket code.
 */
class IProtocolSessionManager {
public:
    virtual ~IProtocolSessionManager() = default;

    /**
     * @brief Leases or creates a ProtocolSession for a given adapter, protocol, and target endpoint.
     */
    virtual std::optional<ProtocolSession> AcquireSession(
        const std::string& adapterId,
        const std::string& protocol,
        const std::string& targetEndpoint,
        uint32_t timeoutMs = 5000) = 0;

    /**
     * @brief Releases a leased ProtocolSession.
     */
    virtual void ReleaseSession(const ProtocolSession& session) = 0;

    /**
     * @brief Looks up a ProtocolSession by sessionId.
     */
    virtual std::optional<ProtocolSession> FindSession(const ProtocolSessionId& sessionId) const = 0;

    /**
     * @brief Looks up the mutable ProtocolSessionContext for a session.
     */
    virtual std::optional<ProtocolSessionContext> GetSessionContext(const ProtocolSessionId& sessionId) const = 0;

    /**
     * @brief Invalidates a session (marking authentication/session state invalid).
     */
    virtual void InvalidateSession(const ProtocolSessionId& sessionId) = 0;

    /**
     * @brief Refreshes last activity and extends expiration for a session.
     */
    virtual bool RefreshSession(const ProtocolSessionId& sessionId) = 0;

    /**
     * @brief Closes and destroys a session by sessionId.
     */
    virtual void CloseSession(const ProtocolSessionId& sessionId) = 0;

    /**
     * @brief Closes all active sessions and resets manager state.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Returns telemetry statistics.
     */
    virtual const ProtocolSessionStatistics& GetStatistics() const = 0;
};

} // namespace Protocol
} // namespace NetDiscovery
