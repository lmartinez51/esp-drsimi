/**
 * @file ExecutionClock.h
 * @brief Abstract execution clock interface and implementations for dependency injection (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

#include <cstdint>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Abstract time source injected into RuntimeExecutionEngine.
 *
 * Decouples the engine from direct std::chrono or esp_timer dependencies,
 * enabling deterministic unit testing and portable host-side simulation.
 */
class IExecutionClock {
public:
    virtual ~IExecutionClock() = default;

    /**
     * @brief Returns the current time as milliseconds since an arbitrary epoch.
     */
    virtual uint64_t NowMs() const = 0;

    /**
     * @brief Returns elapsed milliseconds between two NowMs() readings.
     */
    virtual uint64_t ElapsedMs(uint64_t startMs) const = 0;
};

// ---------------------------------------------------------------------------
// SystemExecutionClock — backed by esp_timer_get_time()
// ---------------------------------------------------------------------------

/**
 * @brief Production clock backed by esp_timer_get_time() converted to milliseconds.
 *
 * Declared here but implemented in SystemExecutionClock.cpp to avoid pulling
 * esp_timer.h into every translation unit that includes this header.
 */
class SystemExecutionClock : public IExecutionClock {
public:
    SystemExecutionClock() = default;
    ~SystemExecutionClock() override = default;

    uint64_t NowMs() const override;
    uint64_t ElapsedMs(uint64_t startMs) const override;
};

// ---------------------------------------------------------------------------
// MockExecutionClock — manually advanced for deterministic testing
// ---------------------------------------------------------------------------

/**
 * @brief Test/simulation clock with a manually settable time value.
 *
 * The test harness advances time by calling Advance() or SetNow().
 * The engine sees a fully deterministic timeline with no hardware dependency.
 */
class MockExecutionClock : public IExecutionClock {
public:
    explicit MockExecutionClock(uint64_t initialMs = 0) : m_nowMs(initialMs) {}
    ~MockExecutionClock() override = default;

    uint64_t NowMs() const override { return m_nowMs; }

    uint64_t ElapsedMs(uint64_t startMs) const override {
        return (m_nowMs >= startMs) ? (m_nowMs - startMs) : 0;
    }

    void SetNow(uint64_t ms) { m_nowMs = ms; }
    void Advance(uint64_t deltaMs) { m_nowMs += deltaMs; }

private:
    uint64_t m_nowMs;
};

} // namespace Runtime
} // namespace NetDiscovery
