/**
 * @file MQTTAdapterStatistics.h
 * @brief Telemetry metrics structure for MQTTAdapter (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Thread-safe telemetry statistics container owned by MQTTAdapter.
 */
struct MQTTAdapterStatistics {
    std::atomic<uint64_t> requestsExecuted{0};
    std::atomic<uint64_t> publishesExecuted{0};
    std::atomic<uint64_t> subscribesExecuted{0};
    std::atomic<uint64_t> unsubscribesExecuted{0};
    std::atomic<uint64_t> disconnectsExecuted{0};
    std::atomic<uint64_t> successfulCalls{0};
    std::atomic<uint64_t> failedCalls{0};
    std::atomic<uint64_t> reconnects{0};
    std::atomic<uint64_t> retriesAttempted{0};
    std::atomic<uint64_t> protocolErrors{0};
    std::atomic<uint64_t> transportErrors{0};
    std::atomic<uint64_t> averageLatencyMs{0};
    std::atomic<uint64_t> bytesTransmitted{0};
    std::atomic<uint64_t> bytesReceived{0};

    void RecordRequest() { requestsExecuted.fetch_add(1, std::memory_order_relaxed); }
    void RecordPublish() { publishesExecuted.fetch_add(1, std::memory_order_relaxed); }
    void RecordSubscribe() { subscribesExecuted.fetch_add(1, std::memory_order_relaxed); }
    void RecordUnsubscribe() { unsubscribesExecuted.fetch_add(1, std::memory_order_relaxed); }
    void RecordDisconnect() { disconnectsExecuted.fetch_add(1, std::memory_order_relaxed); }
    void RecordSuccess() { successfulCalls.fetch_add(1, std::memory_order_relaxed); }
    void RecordFailure() { failedCalls.fetch_add(1, std::memory_order_relaxed); }
    void RecordReconnect() { reconnects.fetch_add(1, std::memory_order_relaxed); }
    void RecordRetry()   { retriesAttempted.fetch_add(1, std::memory_order_relaxed); }
    void RecordProtocolError() { protocolErrors.fetch_add(1, std::memory_order_relaxed); }
    void RecordTransportError() { transportErrors.fetch_add(1, std::memory_order_relaxed); }

    void RecordBytes(uint64_t tx, uint64_t rx) {
        bytesTransmitted.fetch_add(tx, std::memory_order_relaxed);
        bytesReceived.fetch_add(rx, std::memory_order_relaxed);
    }

    void RecordLatency(uint64_t latencyMs) {
        uint64_t prevAvg = averageLatencyMs.load(std::memory_order_relaxed);
        uint64_t newAvg  = (prevAvg == 0) ? latencyMs : (prevAvg * 7 + latencyMs) / 8;
        averageLatencyMs.store(newAvg, std::memory_order_relaxed);
    }
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
