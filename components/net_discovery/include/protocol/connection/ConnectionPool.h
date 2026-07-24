/**
 * @file ConnectionPool.h
 * @brief Reusable, thread-safe connection pool implementing IConnectionManager (v5.0.0 Architecture Phase 11.2).
 */

#pragma once

#include "protocol/connection/IConnectionManager.h"
#include "runtime/ExecutionClock.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Thread-safe connection pool managing reusable ConnectionHandle instances.
 *
 * Enforces maximum pool limits, idle timeouts, and connection reuse.
 * Contains ZERO protocol-specific or socket-specific logic.
 */
class ConnectionPool : public IConnectionManager {
public:
    explicit ConnectionPool(uint32_t maxPoolSize = 16,
                            uint32_t defaultIdleTimeoutMs = 30000,
                            std::shared_ptr<Runtime::IExecutionClock> clock = nullptr);

    ~ConnectionPool() override = default;

    // ── IConnectionManager ───────────────────────────────────────────────────

    std::optional<ConnectionHandle> AcquireConnection(
        const std::string& protocol,
        const std::string& endpoint,
        uint32_t timeoutMs = 5000) override;

    void ReleaseConnection(const ConnectionHandle& handle) override;
    bool ValidateConnection(const ConnectionHandle& handle) override;
    void CloseConnection(const ConnectionId& connectionId) override;
    uint32_t CloseIdleConnections(uint32_t maxIdleTimeMs) override;
    void Shutdown() override;

    const ConnectionStatistics& GetStatistics() const override { return m_stats; }

private:
    struct PooledRecord {
        ConnectionHandle handle;
        ConnectionStatus status{ConnectionStatus::Connected};
        bool             inUse{false};
    };

    uint32_t m_maxPoolSize{16};
    uint32_t m_defaultIdleTimeoutMs{30000};
    std::shared_ptr<Runtime::IExecutionClock> m_clock;

    ConnectionStatistics m_stats;

    mutable std::mutex m_mutex;
    std::unordered_map<ConnectionId, PooledRecord> m_pool;
    uint64_t m_nextId{1};
};

} // namespace Protocol
} // namespace NetDiscovery
