/**
 * @file RuntimeDiagnostics.h
 * @brief Pure-counter runtime statistics collector (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

#include <cstdint>
#include <atomic>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Accumulates atomic runtime counters for the execution engine.
 *
 * No logging. No printing. No I/O. Only monotonically incrementing counters.
 * Observers read counters directly; RuntimeExecutionEngine writes them.
 *
 * Designed for single-owner (RuntimeExecutionEngine) with multiple read-only observers.
 * All write operations use relaxed atomics for embedded performance.
 */
class RuntimeDiagnostics {
public:
    RuntimeDiagnostics() = default;
    ~RuntimeDiagnostics() = default;

    // Non-copyable — diagnostics are owned by a single engine instance.
    RuntimeDiagnostics(const RuntimeDiagnostics&) = delete;
    RuntimeDiagnostics& operator=(const RuntimeDiagnostics&) = delete;

    // ── Session Lifecycle ───────────────────────────────────────────────────
    void RecordSessionCreated()   { m_sessionsCreated.fetch_add(1, std::memory_order_relaxed); }
    void RecordSessionCompleted() { m_sessionsCompleted.fetch_add(1, std::memory_order_relaxed); }
    void RecordSessionCancelled() { m_sessionsCancelled.fetch_add(1, std::memory_order_relaxed); }
    void RecordSessionFailed()    { m_sessionsFailed.fetch_add(1, std::memory_order_relaxed); }
    void RecordSessionTimeout()   { m_sessionsTimedOut.fetch_add(1, std::memory_order_relaxed); }

    // ── Step Execution ──────────────────────────────────────────────────────
    void RecordDispatcherCall()   { m_dispatcherCalls.fetch_add(1, std::memory_order_relaxed); }
    void RecordStepRetry()        { m_totalRetries.fetch_add(1, std::memory_order_relaxed); }
    void RecordRollback()         { m_totalRollbacks.fetch_add(1, std::memory_order_relaxed); }

    // ── Tick Timing ─────────────────────────────────────────────────────────
    /**
     * @brief Records a single Tick() duration and updates the running average.
     * @param tickDurationMs Duration of the last Tick() in milliseconds.
     */
    void RecordTickDuration(uint32_t tickDurationMs) {
        m_tickCount.fetch_add(1, std::memory_order_relaxed);
        // Incremental running average: avg_n = avg_{n-1} + (x_n - avg_{n-1}) / n
        uint64_t n = m_tickCount.load(std::memory_order_relaxed);
        uint64_t prev = m_averageTickDurationMs.load(std::memory_order_relaxed);
        uint64_t next = prev + (static_cast<uint64_t>(tickDurationMs) - prev) / n;
        m_averageTickDurationMs.store(next, std::memory_order_relaxed);
    }

    // ── Read Accessors ──────────────────────────────────────────────────────
    uint64_t GetSessionsCreated()   const { return m_sessionsCreated.load(std::memory_order_relaxed); }
    uint64_t GetSessionsCompleted() const { return m_sessionsCompleted.load(std::memory_order_relaxed); }
    uint64_t GetSessionsCancelled() const { return m_sessionsCancelled.load(std::memory_order_relaxed); }
    uint64_t GetSessionsFailed()    const { return m_sessionsFailed.load(std::memory_order_relaxed); }
    uint64_t GetSessionsTimedOut()  const { return m_sessionsTimedOut.load(std::memory_order_relaxed); }
    uint64_t GetDispatcherCalls()   const { return m_dispatcherCalls.load(std::memory_order_relaxed); }
    uint64_t GetTotalRetries()      const { return m_totalRetries.load(std::memory_order_relaxed); }
    uint64_t GetTotalRollbacks()    const { return m_totalRollbacks.load(std::memory_order_relaxed); }
    uint64_t GetTickCount()         const { return m_tickCount.load(std::memory_order_relaxed); }
    uint64_t GetAverageTickDurationMs() const { return m_averageTickDurationMs.load(std::memory_order_relaxed); }

    // ── Reset ───────────────────────────────────────────────────────────────
    void Reset() {
        m_sessionsCreated.store(0, std::memory_order_relaxed);
        m_sessionsCompleted.store(0, std::memory_order_relaxed);
        m_sessionsCancelled.store(0, std::memory_order_relaxed);
        m_sessionsFailed.store(0, std::memory_order_relaxed);
        m_sessionsTimedOut.store(0, std::memory_order_relaxed);
        m_dispatcherCalls.store(0, std::memory_order_relaxed);
        m_totalRetries.store(0, std::memory_order_relaxed);
        m_totalRollbacks.store(0, std::memory_order_relaxed);
        m_tickCount.store(0, std::memory_order_relaxed);
        m_averageTickDurationMs.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> m_sessionsCreated{0};
    std::atomic<uint64_t> m_sessionsCompleted{0};
    std::atomic<uint64_t> m_sessionsCancelled{0};
    std::atomic<uint64_t> m_sessionsFailed{0};
    std::atomic<uint64_t> m_sessionsTimedOut{0};
    std::atomic<uint64_t> m_dispatcherCalls{0};
    std::atomic<uint64_t> m_totalRetries{0};
    std::atomic<uint64_t> m_totalRollbacks{0};
    std::atomic<uint64_t> m_tickCount{0};
    std::atomic<uint64_t> m_averageTickDurationMs{0};
};

} // namespace Runtime
} // namespace NetDiscovery
