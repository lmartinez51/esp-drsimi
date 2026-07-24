/**
 * @file ExecutionSessionRepository.h
 * @brief Thread-safe O(1) repository owning the lifetime of all ExecutionSessions (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

#include "execution/ExecutionSession.h"
#include "execution/ExecutionPlan.h"
#include "execution/ExecutionPlanState.h"

#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Repository owning session lifetime and providing O(1) lookup by SessionId.
 *
 * RuntimeExecutionEngine no longer owns sessions directly. It delegates session
 * storage to ExecutionSessionRepository, which is injected via constructor.
 * No singleton. No global state.
 *
 * Thread-safety model: all public methods acquire m_mutex. Returned copies are
 * independent of internal state after the lock is released.
 */
class ExecutionSessionRepository {
public:
    ExecutionSessionRepository() = default;
    ~ExecutionSessionRepository() = default;

    // Non-copyable — the repository is the single owner.
    ExecutionSessionRepository(const ExecutionSessionRepository&) = delete;
    ExecutionSessionRepository& operator=(const ExecutionSessionRepository&) = delete;

    // Movable — allows embedding in RuntimeExecutionEngine or transferring ownership.
    ExecutionSessionRepository(ExecutionSessionRepository&&) = default;
    ExecutionSessionRepository& operator=(ExecutionSessionRepository&&) = default;

    // ── Write Operations ────────────────────────────────────────────────────

    /**
     * @brief Registers an (ExecutionPlan, ExecutionSession) pair into the repository.
     *
     * The repository takes ownership. If a session with the same SessionId already
     * exists it is replaced.
     */
    void Register(Execution::ExecutionPlan plan, Execution::ExecutionSession session);

    /**
     * @brief Removes a session and its associated plan by SessionId.
     * @return true if the session was found and removed.
     */
    bool Remove(const Execution::SessionId& sessionId);

    // ── Read Operations ─────────────────────────────────────────────────────

    /**
     * @brief Returns a copy of the session if registered. O(1).
     */
    std::optional<Execution::ExecutionSession> FindSession(const Execution::SessionId& sessionId) const;

    /**
     * @brief Returns a copy of the plan associated with a session. O(1).
     */
    std::optional<Execution::ExecutionPlan> FindPlan(const Execution::SessionId& sessionId) const;

    /**
     * @brief Returns true if the repository contains an entry for sessionId.
     */
    bool Contains(const Execution::SessionId& sessionId) const;

    /**
     * @brief Returns the current count of registered sessions.
     */
    std::size_t Count() const;

    // ── Enumeration ─────────────────────────────────────────────────────────

    /**
     * @brief Returns a snapshot of all registered SessionIds.
     */
    std::vector<Execution::SessionId> GetAllSessionIds() const;

    /**
     * @brief Returns SessionIds of all non-terminal sessions (Ready, Running, Waiting, Paused).
     */
    std::vector<Execution::SessionId> GetActiveSessions() const;

    /**
     * @brief Returns SessionIds of sessions in state Running.
     */
    std::vector<Execution::SessionId> GetRunningSessions() const;

    /**
     * @brief Returns SessionIds of sessions in terminal states (Completed, Failed, Cancelled, etc.).
     */
    std::vector<Execution::SessionId> GetCompletedSessions() const;

    // ── Mutable Session Access ──────────────────────────────────────────────

    /**
     * @brief Provides mutable access to (plan, session) context via a callback.
     *
     * The callback is invoked while the mutex is held; it must not call back
     * into ExecutionSessionRepository to avoid deadlock.
     *
     * @param sessionId Target session.
     * @param fn        Callback: void(ExecutionPlan&, ExecutionSession&)
     * @return true if the session was found and the callback was invoked.
     */
    template <typename Fn>
    bool WithSession(const Execution::SessionId& sessionId, Fn&& fn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_store.find(sessionId);
        if (it == m_store.end()) return false;
        fn(it->second.plan, it->second.session);
        return true;
    }

private:
    struct SessionContext {
        Execution::ExecutionPlan    plan;
        Execution::ExecutionSession session;

        SessionContext(Execution::ExecutionPlan p, Execution::ExecutionSession s)
            : plan(std::move(p)), session(std::move(s)) {}
    };

    mutable std::mutex m_mutex;
    std::unordered_map<Execution::SessionId, SessionContext> m_store;
};

} // namespace Runtime
} // namespace NetDiscovery
