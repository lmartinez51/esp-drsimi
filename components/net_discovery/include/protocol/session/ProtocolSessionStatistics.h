/**
 * @file ProtocolSessionStatistics.h
 * @brief Telemetry metrics structure for protocol sessions (v5.0.0 Architecture Phase 12.1).
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Thread-safe telemetry statistics container for protocol session management.
 */
struct ProtocolSessionStatistics {
    std::atomic<uint64_t> sessionsCreated{0};
    std::atomic<uint64_t> sessionsDestroyed{0};
    std::atomic<uint64_t> activeSessions{0};
    std::atomic<uint64_t> reusedSessions{0};
    std::atomic<uint64_t> expiredSessions{0};
    std::atomic<uint64_t> authenticationFailures{0};
    std::atomic<uint64_t> reconnectCount{0};
    std::atomic<uint64_t> averageLifetimeMs{0};

    void RecordCreated() {
        sessionsCreated.fetch_add(1, std::memory_order_relaxed);
        activeSessions.fetch_add(1, std::memory_order_relaxed);
    }

    void RecordReused() {
        reusedSessions.fetch_add(1, std::memory_order_relaxed);
    }

    void RecordDestroyed() {
        sessionsDestroyed.fetch_add(1, std::memory_order_relaxed);
        uint64_t curr = activeSessions.load(std::memory_order_relaxed);
        if (curr > 0) {
            activeSessions.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    void RecordExpired() {
        expiredSessions.fetch_add(1, std::memory_order_relaxed);
        RecordDestroyed();
    }

    void RecordAuthFailure() {
        authenticationFailures.fetch_add(1, std::memory_order_relaxed);
    }

    void RecordReconnect() {
        reconnectCount.fetch_add(1, std::memory_order_relaxed);
    }
};

} // namespace Protocol
} // namespace NetDiscovery
