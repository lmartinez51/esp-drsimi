/**
 * @file ProtocolSessionManager.h
 * @brief Reusable, thread-safe session manager implementing IProtocolSessionManager (v5.0.0 Architecture Phase 12.1).
 */

#pragma once

#include "protocol/session/IProtocolSessionManager.h"
#include "runtime/ExecutionClock.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Thread-safe implementation of IProtocolSessionManager.
 *
 * Owns every ProtocolSession and ProtocolSessionContext. Handles session creation,
 * expiration, reuse, and authentication state transitions.
 * Contains ZERO protocol code, ZERO networking, and ZERO socket logic.
 */
class ProtocolSessionManager : public IProtocolSessionManager {
public:
    explicit ProtocolSessionManager(uint32_t maxSessions = 32,
                                    uint32_t defaultSessionTimeoutMs = 600000,
                                    std::shared_ptr<Runtime::IExecutionClock> clock = nullptr);

    ~ProtocolSessionManager() override = default;

    // ── IProtocolSessionManager ──────────────────────────────────────────────

    std::optional<ProtocolSession> AcquireSession(
        const std::string& adapterId,
        const std::string& protocol,
        const std::string& targetEndpoint,
        uint32_t timeoutMs = 5000) override;

    void ReleaseSession(const ProtocolSession& session) override;
    std::optional<ProtocolSession> FindSession(const ProtocolSessionId& sessionId) const override;
    std::optional<ProtocolSessionContext> GetSessionContext(const ProtocolSessionId& sessionId) const override;
    void InvalidateSession(const ProtocolSessionId& sessionId) override;
    bool RefreshSession(const ProtocolSessionId& sessionId) override;
    void CloseSession(const ProtocolSessionId& sessionId) override;
    void Shutdown() override;

    const ProtocolSessionStatistics& GetStatistics() const override { return m_stats; }

private:
    struct SessionRecord {
        ProtocolSession        session;
        ProtocolSessionContext context;
        bool                   inUse{false};
    };

    uint32_t m_maxSessions{32};
    uint32_t m_defaultSessionTimeoutMs{600000};
    std::shared_ptr<Runtime::IExecutionClock> m_clock;

    ProtocolSessionStatistics m_stats;

    mutable std::mutex m_mutex;
    std::unordered_map<ProtocolSessionId, SessionRecord> m_sessions;
    uint64_t m_nextId{1};
};

} // namespace Protocol
} // namespace NetDiscovery
