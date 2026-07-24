/**
 * @file ConnectionPool.cpp
 * @brief Implementation of ConnectionPool (v5.0.0 Architecture Phase 11.2).
 */

#include "protocol/connection/ConnectionPool.h"

#include <algorithm>

namespace NetDiscovery {
namespace Protocol {

ConnectionPool::ConnectionPool(uint32_t maxPoolSize,
                               uint32_t defaultIdleTimeoutMs,
                               std::shared_ptr<Runtime::IExecutionClock> clock)
    : m_maxPoolSize(maxPoolSize)
    , m_defaultIdleTimeoutMs(defaultIdleTimeoutMs)
    , m_clock(std::move(clock)) {

    if (!m_clock) {
        m_clock = std::make_shared<Runtime::SystemExecutionClock>();
    }
}

std::optional<ConnectionHandle> ConnectionPool::AcquireConnection(
        const std::string& protocol,
        const std::string& endpoint,
        uint32_t /*timeoutMs*/) {

    std::lock_guard<std::mutex> lock(m_mutex);
    const uint64_t now = m_clock->NowMs();

    // 1. Search for existing idle connection matching protocol and endpoint
    for (auto& kv : m_pool) {
        auto& rec = kv.second;
        if (!rec.inUse && rec.status == ConnectionStatus::Connected &&
            rec.handle.protocol == protocol && rec.handle.endpoint == endpoint) {

            rec.inUse = true;
            rec.handle.lastActivityTimestampMs = now;
            m_stats.RecordReused();
            return rec.handle;
        }
    }

    // 2. Check max pool size limit
    if (m_pool.size() >= m_maxPoolSize) {
        // Try cleaning up idle connections first
        uint32_t closed = 0;
        for (auto it = m_pool.begin(); it != m_pool.end(); ) {
            if (!it->second.inUse) {
                m_stats.RecordDestroyed();
                it = m_pool.erase(it);
                ++closed;
                if (m_pool.size() < m_maxPoolSize) break;
            } else {
                ++it;
            }
        }
        if (m_pool.size() >= m_maxPoolSize) {
            m_stats.RecordFailed();
            return std::nullopt;
        }
    }

    // 3. Create new connection handle record
    ConnectionId cid = "conn." + protocol + "." + std::to_string(m_nextId++);
    ConnectionHandle handle(cid, protocol, endpoint, now, now, 1);

    PooledRecord rec;
    rec.handle = handle;
    rec.status = ConnectionStatus::Connected;
    rec.inUse  = true;

    m_pool.emplace(cid, rec);
    m_stats.RecordCreated();

    return handle;
}

void ConnectionPool::ReleaseConnection(const ConnectionHandle& handle) {
    if (!handle.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pool.find(handle.connectionId);
    if (it != m_pool.end()) {
        it->second.inUse = false;
        it->second.handle.lastActivityTimestampMs = m_clock->NowMs();
    }
}

bool ConnectionPool::ValidateConnection(const ConnectionHandle& handle) {
    if (!handle.IsValid()) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pool.find(handle.connectionId);
    if (it == m_pool.end()) return false;
    return it->second.status == ConnectionStatus::Connected;
}

void ConnectionPool::CloseConnection(const ConnectionId& connectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_pool.find(connectionId);
    if (it != m_pool.end()) {
        m_stats.RecordDestroyed();
        m_pool.erase(it);
    }
}

uint32_t ConnectionPool::CloseIdleConnections(uint32_t maxIdleTimeMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const uint64_t now = m_clock->NowMs();
    const uint32_t threshold = maxIdleTimeMs > 0 ? maxIdleTimeMs : m_defaultIdleTimeoutMs;
    uint32_t closedCount = 0;

    for (auto it = m_pool.begin(); it != m_pool.end(); ) {
        if (!it->second.inUse && (now - it->second.handle.lastActivityTimestampMs > threshold)) {
            m_stats.RecordDestroyed();
            it = m_pool.erase(it);
            ++closedCount;
        } else {
            ++it;
        }
    }
    return closedCount;
}

void ConnectionPool::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (std::size_t i = 0; i < m_pool.size(); ++i) {
        m_stats.RecordDestroyed();
    }
    m_pool.clear();
}


} // namespace Protocol
} // namespace NetDiscovery
