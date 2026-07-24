/**
 * @file ExecutionSessionRepository.cpp
 * @brief Implementation of ExecutionSessionRepository (v5.0.0 Architecture Phase 9.2).
 */

#include "runtime/ExecutionSessionRepository.h"

#include <algorithm>

namespace NetDiscovery {
namespace Runtime {

void ExecutionSessionRepository::Register(Execution::ExecutionPlan plan,
                                           Execution::ExecutionSession session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Execution::SessionId sid = session.sessionId;
    m_store.insert_or_assign(std::move(sid),
                             SessionContext(std::move(plan), std::move(session)));
}

bool ExecutionSessionRepository::Remove(const Execution::SessionId& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_store.erase(sessionId) > 0;
}

std::optional<Execution::ExecutionSession>
ExecutionSessionRepository::FindSession(const Execution::SessionId& sessionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_store.find(sessionId);
    if (it == m_store.end()) return std::nullopt;
    return it->second.session;
}

std::optional<Execution::ExecutionPlan>
ExecutionSessionRepository::FindPlan(const Execution::SessionId& sessionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_store.find(sessionId);
    if (it == m_store.end()) return std::nullopt;
    return it->second.plan;
}

bool ExecutionSessionRepository::Contains(const Execution::SessionId& sessionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_store.count(sessionId) > 0;
}

std::size_t ExecutionSessionRepository::Count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_store.size();
}

std::vector<Execution::SessionId> ExecutionSessionRepository::GetAllSessionIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Execution::SessionId> ids;
    ids.reserve(m_store.size());
    for (const auto& kv : m_store) {
        ids.push_back(kv.first);
    }
    return ids;
}

std::vector<Execution::SessionId> ExecutionSessionRepository::GetActiveSessions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Execution::SessionId> ids;
    for (const auto& kv : m_store) {
        if (kv.second.session.IsActive()) {
            ids.push_back(kv.first);
        }
    }
    return ids;
}

std::vector<Execution::SessionId> ExecutionSessionRepository::GetRunningSessions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Execution::SessionId> ids;
    for (const auto& kv : m_store) {
        if (kv.second.session.currentState == Execution::ExecutionPlanState::Running) {
            ids.push_back(kv.first);
        }
    }
    return ids;
}

std::vector<Execution::SessionId> ExecutionSessionRepository::GetCompletedSessions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Execution::SessionId> ids;
    for (const auto& kv : m_store) {
        if (kv.second.session.IsTerminal()) {
            ids.push_back(kv.first);
        }
    }
    return ids;
}

} // namespace Runtime
} // namespace NetDiscovery
