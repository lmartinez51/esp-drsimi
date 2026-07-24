/**
 * @file CancellationToken.h
 * @brief Thread-safe cancellation token wrapper (v6.0 Phase C).
 */

#pragma once

#include <atomic>
#include <memory>

namespace NetDiscovery {
namespace Plan {

class CancellationToken {
public:
    CancellationToken()
        : m_cancelled(std::make_shared<std::atomic<bool>>(false)) {}

    explicit CancellationToken(std::shared_ptr<std::atomic<bool>> flag)
        : m_cancelled(std::move(flag)) {
        if (!m_cancelled) {
            m_cancelled = std::make_shared<std::atomic<bool>>(false);
        }
    }

    bool IsCancelled() const {
        return m_cancelled && m_cancelled->load();
    }

    void Cancel() {
        if (m_cancelled) {
            m_cancelled->store(true);
        }
    }

    std::shared_ptr<std::atomic<bool>> GetAtomicFlag() const {
        return m_cancelled;
    }

private:
    std::shared_ptr<std::atomic<bool>> m_cancelled;
};

} // namespace Plan
} // namespace NetDiscovery
