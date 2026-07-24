/**
 * @file ExecutionEventQueue.h
 * @brief Simple STL-backed FIFO runtime event queue (v5.0.0 Architecture Phase 9.1).
 */

#pragma once

#include "runtime/ExecutionEvent.h"

#include <queue>
#include <optional>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Pure STL FIFO queue holding pending runtime events for an ExecutionSession.
 */
class ExecutionEventQueue {
public:
    ExecutionEventQueue() = default;
    ~ExecutionEventQueue() = default;

    void Push(ExecutionEvent event) {
        m_queue.push(std::move(event));
    }

    std::optional<ExecutionEvent> Pop() {
        if (m_queue.empty()) return std::nullopt;
        ExecutionEvent evt = std::move(m_queue.front());
        m_queue.pop();
        return evt;
    }

    const ExecutionEvent* Peek() const {
        if (m_queue.empty()) return nullptr;
        return &m_queue.front();
    }

    bool Empty() const {
        return m_queue.empty();
    }

    size_t Size() const {
        return m_queue.size();
    }

    void Clear() {
        std::queue<ExecutionEvent> emptyQueue;
        std::swap(m_queue, emptyQueue);
    }

private:
    std::queue<ExecutionEvent> m_queue;
};

} // namespace Runtime
} // namespace NetDiscovery
