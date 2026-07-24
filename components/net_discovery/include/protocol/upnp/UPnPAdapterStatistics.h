/**
 * @file UPnPAdapterStatistics.h
 * @brief Dedicated telemetry container for UPnPAdapter metrics (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Thread-safe telemetry statistics container owned by UPnPAdapter.
 */
struct UPnPAdapterStatistics {
    std::atomic<uint64_t> requestsExecuted{0};
    std::atomic<uint64_t> successfulCalls{0};
    std::atomic<uint64_t> failedCalls{0};
    std::atomic<uint64_t> retriesAttempted{0};
    std::atomic<uint64_t> averageLatencyMs{0};
    std::atomic<uint64_t> parserFailures{0};
    std::atomic<uint64_t> transportFailures{0};
    std::atomic<uint64_t> soapFaults{0};

    void RecordRequest() { requestsExecuted.fetch_add(1, std::memory_order_relaxed); }
    void RecordSuccess() { successfulCalls.fetch_add(1, std::memory_order_relaxed); }
    void RecordFailure() { failedCalls.fetch_add(1, std::memory_order_relaxed); }
    void RecordRetry()   { retriesAttempted.fetch_add(1, std::memory_order_relaxed); }
    void RecordParserError() { parserFailures.fetch_add(1, std::memory_order_relaxed); }
    void RecordTransportError() { transportFailures.fetch_add(1, std::memory_order_relaxed); }
    void RecordSoapFault() { soapFaults.fetch_add(1, std::memory_order_relaxed); }

    void RecordLatency(uint64_t latencyMs) {
        uint64_t prevAvg = averageLatencyMs.load(std::memory_order_relaxed);
        uint64_t newAvg  = (prevAvg == 0) ? latencyMs : (prevAvg * 7 + latencyMs) / 8;
        averageLatencyMs.store(newAvg, std::memory_order_relaxed);
    }
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
