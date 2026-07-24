/**
 * @file HTTPAdapterStatistics.h
 * @brief Telemetry metrics structure for HTTPAdapter (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Thread-safe telemetry statistics container owned by HTTPAdapter.
 */
struct HTTPAdapterStatistics {
    std::atomic<uint64_t> requestsExecuted{0};
    std::atomic<uint64_t> getRequests{0};
    std::atomic<uint64_t> postRequests{0};
    std::atomic<uint64_t> putRequests{0};
    std::atomic<uint64_t> deleteRequests{0};
    std::atomic<uint64_t> patchRequests{0};
    std::atomic<uint64_t> successfulCalls{0};
    std::atomic<uint64_t> failedCalls{0};
    std::atomic<uint64_t> retriesAttempted{0};
    std::atomic<uint64_t> bytesSent{0};
    std::atomic<uint64_t> bytesReceived{0};
    std::atomic<uint64_t> averageLatencyMs{0};

    void RecordRequest() { requestsExecuted.fetch_add(1, std::memory_order_relaxed); }
    void RecordGet() { getRequests.fetch_add(1, std::memory_order_relaxed); }
    void RecordPost() { postRequests.fetch_add(1, std::memory_order_relaxed); }
    void RecordPut() { putRequests.fetch_add(1, std::memory_order_relaxed); }
    void RecordDelete() { deleteRequests.fetch_add(1, std::memory_order_relaxed); }
    void RecordPatch() { patchRequests.fetch_add(1, std::memory_order_relaxed); }
    void RecordSuccess() { successfulCalls.fetch_add(1, std::memory_order_relaxed); }
    void RecordFailure() { failedCalls.fetch_add(1, std::memory_order_relaxed); }
    void RecordRetry()   { retriesAttempted.fetch_add(1, std::memory_order_relaxed); }

    void RecordBytes(uint64_t sent, uint64_t recv) {
        bytesSent.fetch_add(sent, std::memory_order_relaxed);
        bytesReceived.fetch_add(recv, std::memory_order_relaxed);
    }

    void RecordLatency(uint64_t latencyMs) {
        uint64_t prevAvg = averageLatencyMs.load(std::memory_order_relaxed);
        uint64_t newAvg  = (prevAvg == 0) ? latencyMs : (prevAvg * 7 + latencyMs) / 8;
        averageLatencyMs.store(newAvg, std::memory_order_relaxed);
    }
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
