/**
 * @file MockEventSignal.h
 * @brief Controllable test double implementation of IEventSignal (v6.0 Phase D).
 */

#pragma once

#include "plan/events/IEventSignal.h"
#include <atomic>
#include <thread>

namespace NetDiscovery {
namespace Plan {

class MockEventSignal : public IEventSignal {
public:
    explicit MockEventSignal(bool initialSignalled = false)
        : m_signalled(initialSignalled) {}

    WaitResult Wait(std::chrono::milliseconds timeout, CancellationToken cancelToken) override {
        auto start = std::chrono::steady_clock::now();
        while (!m_signalled.load()) {
            if (cancelToken.IsCancelled()) {
                return WaitResult::Cancelled;
            }
            if (timeout.count() > 0 && (std::chrono::steady_clock::now() - start) >= timeout) {
                return WaitResult::TimedOut;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return WaitResult::Signalled;
    }

    void Signal() override {
        m_signalled.store(true);
    }

    void Reset() override {
        m_signalled.store(false);
    }

private:
    std::atomic<bool> m_signalled;
};

} // namespace Plan
} // namespace NetDiscovery
