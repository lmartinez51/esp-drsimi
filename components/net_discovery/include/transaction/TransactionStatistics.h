/**
 * @file TransactionStatistics.h
 * @brief Thread-safe telemetry statistics container for transactions (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace NetDiscovery {
namespace Transaction {

/**
 * @brief Atomic telemetry counters for transaction management.
 */
struct TransactionStatistics {
    std::atomic<uint64_t> transactionsCreated{0};
    std::atomic<uint64_t> committed{0};
    std::atomic<uint64_t> aborted{0};
    std::atomic<uint64_t> failed{0};
    std::atomic<uint64_t> timedOut{0};
    std::atomic<uint64_t> averageLifetimeMs{0};
    std::atomic<uint64_t> averageCommitTimeMs{0};

    void RecordCreated() { transactionsCreated.fetch_add(1, std::memory_order_relaxed); }
    void RecordCommitted() { committed.fetch_add(1, std::memory_order_relaxed); }
    void RecordAborted() { aborted.fetch_add(1, std::memory_order_relaxed); }
    void RecordFailed() { failed.fetch_add(1, std::memory_order_relaxed); }
    void RecordTimedOut() { timedOut.fetch_add(1, std::memory_order_relaxed); }

    void RecordLifetime(uint64_t lifetimeMs) {
        uint64_t prev = averageLifetimeMs.load(std::memory_order_relaxed);
        uint64_t next = (prev == 0) ? lifetimeMs : (prev * 7 + lifetimeMs) / 8;
        averageLifetimeMs.store(next, std::memory_order_relaxed);
    }
};

} // namespace Transaction
} // namespace NetDiscovery
