/**
 * @file ProtocolSessionManager.cpp
 * @brief Implementation of ProtocolSessionManager (v5.0.0 Architecture Phase 12.1).
 */

#include "protocol/session/ProtocolSessionManager.h"

namespace NetDiscovery {
namespace Protocol {

ProtocolSessionManager::ProtocolSessionManager(uint32_t maxSessions,
                                                 uint32_t defaultSessionTimeoutMs,
                                                 std::shared_ptr<Runtime::IExecutionClock> clock)
    : m_maxSessions(maxSessions)
    , m_defaultSessionTimeoutMs(defaultSessionTimeoutMs)
    , m_clock(std::move(clock)) {

    if (!m_clock) {
        m_clock = std::make_shared<Runtime::SystemExecutionClock>();
    }
}

std::optional<ProtocolSession> ProtocolSessionManager::AcquireSession(
        const std::string& adapterId,
        const std::string& protocol,
        const std::string& targetEndpoint,
        uint32_t /*timeoutMs*/) {

    std::lock_guard<std::mutex> lock(m_mutex);
    const uint64_t now = m_clock->NowMs();

    // 1. Look up existing idle session matching adapterId, protocol, and targetEndpoint
    for (auto& kv : m_sessions) {
        auto& rec = kv.second;
        if (!rec.inUse && rec.session.adapterId == adapterId &&
            rec.session.protocol == protocol && rec.session.targetEndpoint == targetEndpoint) {

            rec.inUse = true;
            rec.context.lastActivityTimestampMs = now;
            m_stats.RecordReused();
            return rec.session;
        }
    }

    // 2. Check max sessions limit
    if (m_sessions.size() >= m_maxSessions) {
        // Try cleaning up idle/expired sessions
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
            if (!it->second.inUse) {
                m_stats.RecordDestroyed();
                it = m_sessions.erase(it);
                if (m_sessions.size() < m_maxSessions) break;
            } else {
                ++it;
            }
        }
        if (m_sessions.size() >= m_maxSessions) {
            return std::nullopt;
        }
    }

    // 3. Create new ProtocolSession and ProtocolSessionContext
    ProtocolSessionId sid = "psess." + protocol + "." + std::to_string(m_nextId++);
    ProtocolSession session(sid, adapterId, protocol, targetEndpoint, now, 1);
    ProtocolSessionContext ctx(sid);
    ctx.lastActivityTimestampMs = now;
    ctx.expirationTimestampMs   = now + m_defaultSessionTimeoutMs;

    SessionRecord rec;
    rec.session = session;
    rec.context = ctx;
    rec.inUse   = true;

    m_sessions.emplace(sid, rec);
    m_stats.RecordCreated();

    return session;
}

void ProtocolSessionManager::ReleaseSession(const ProtocolSession& session) {
    if (!session.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(session.sessionId);
    if (it != m_sessions.end()) {
        it->second.inUse = false;
        it->second.context.lastActivityTimestampMs = m_clock->NowMs();
    }
}

std::optional<ProtocolSession> ProtocolSessionManager::FindSession(const ProtocolSessionId& sessionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) return std::nullopt;
    return it->second.session;
}

std::optional<ProtocolSessionContext> ProtocolSessionManager::GetSessionContext(const ProtocolSessionId& sessionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) return std::nullopt;
    return it->second.context;
}

void ProtocolSessionManager::InvalidateSession(const ProtocolSessionId& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if (it != m_sessions.end()) {
        it->second.context.authenticationState = "Invalidated";
        m_stats.RecordAuthFailure();
    }
}

bool ProtocolSessionManager::RefreshSession(const ProtocolSessionId& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) return false;
    const uint64_t now = m_clock->NowMs();
    it->second.context.lastActivityTimestampMs = now;
    it->second.context.expirationTimestampMs   = now + m_defaultSessionTimeoutMs;
    it->second.context.BumpVersion();
    return true;
}

void ProtocolSessionManager::CloseSession(const ProtocolSessionId& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if (it != m_sessions.end()) {
        m_stats.RecordDestroyed();
        m_sessions.erase(it);
    }
}

void ProtocolSessionManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (std::size_t i = 0; i < m_sessions.size(); ++i) {
        m_stats.RecordDestroyed();
    }
    m_sessions.clear();
}

} // namespace Protocol
} // namespace NetDiscovery
