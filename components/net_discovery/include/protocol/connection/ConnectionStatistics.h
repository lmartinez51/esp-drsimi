/**
 * @file ConnectionStatistics.h
 * @brief Telemetry metrics structure for connection management (v5.0.0 Architecture Phase 11.2).
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Thread-safe telemetry metrics for connection pooling and management.
 */
struct ConnectionStatistics {
    std::atomic<uint64_t> activeConnections{0};
    std::atomic<uint64_t> idleConnections{0};
    std::atomic<uint64_t> reusedConnections{0};
    std::atomic<uint64_t> createdConnections{0};
    std::atomic<uint64_t> destroyedConnections{0};
    std::atomic<uint64_t> reconnectCount{0};
    std::atomic<uint64_t> failedConnections{0};
    std::atomic<uint64_t> averageLifetimeMs{0};
    std::atomic<uint64_t> averageIdleTimeMs{0};

    void RecordCreated() {
        createdConnections.fetch_add(1, std::memory_order_relaxed);
        activeConnections.fetch_add(1, std::memory_order_relaxed);
    }

    void RecordReused() {
        reusedConnections.fetch_add(1, std::memory_order_relaxed);
    }

    void RecordDestroyed() {
        destroyedConnections.fetch_add(1, std::memory_order_relaxed);
        uint64_t current = activeConnections.load(std::memory_order_relaxed);
        if (current > 0) {
            activeConnections.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    void RecordFailed() {
        failedConnections.fetch_add(1, std::memory_order_relaxed);
    }

    void RecordReconnect() {
        reconnectCount.fetch_add(1, std::memory_order_relaxed);
    }
};

} // namespace Protocol
} // namespace NetDiscovery
