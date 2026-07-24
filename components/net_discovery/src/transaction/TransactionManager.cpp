/**
 * @file TransactionManager.cpp
 * @brief Implementation of TransactionManager (v5.0.0 Architecture Phase 14).
 */

#include "transaction/TransactionManager.h"

namespace NetDiscovery {
namespace Transaction {

TransactionManager::TransactionManager(uint32_t maxTransactions,
                                         std::shared_ptr<Runtime::IExecutionClock> clock)
    : m_maxTransactions(maxTransactions)
    , m_clock(std::move(clock)) {

    if (!m_clock) {
        m_clock = std::make_shared<Runtime::SystemExecutionClock>();
    }
}

std::optional<ExecutionTransaction> TransactionManager::BeginTransaction(
        const std::string& sessionId,
        const TransactionPolicy& policy) {

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_transactions.size() >= m_maxTransactions) {
        m_stats.RecordFailed();
        return std::nullopt;
    }

    const uint64_t now = m_clock->NowMs();
    TransactionId tid = "tx." + std::to_string(m_nextId++);

    ExecutionTransaction tx(tid, sessionId, now, 1, policy.policyId);
    ExecutionTransactionContext ctx(tid);
    ctx.currentState = TransactionState::Running;
    ctx.startedAtMs = now;

    TransactionRecord rec;
    rec.transaction = tx;
    rec.context     = ctx;
    rec.policy      = policy;

    m_transactions.emplace(tid, rec);
    m_stats.RecordCreated();

    return tx;
}

TransactionResult TransactionManager::Commit(const TransactionId& transactionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transactions.find(transactionId);
    if (it == m_transactions.end()) {
        return TransactionResult(false, TransactionState::Failed,
                                 Runtime::ExecutionFailureReason::ResourceUnavailable,
                                 0, {"Transaction not found: " + transactionId});
    }

    const uint64_t now = m_clock->NowMs();
    auto& rec = it->second;
    rec.context.currentState  = TransactionState::Committed;
    rec.context.committedAtMs = now;

    uint32_t duration = (rec.context.startedAtMs > 0 && now >= rec.context.startedAtMs) ?
                        static_cast<uint32_t>(now - rec.context.startedAtMs) : 0;

    m_stats.RecordCommitted();
    m_stats.RecordLifetime(duration);

    return TransactionResult(true, TransactionState::Committed,
                             Runtime::ExecutionFailureReason::None,
                             duration, {"Transaction committed successfully"});
}

TransactionResult TransactionManager::Abort(const TransactionId& transactionId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transactions.find(transactionId);
    if (it == m_transactions.end()) {
        return TransactionResult(false, TransactionState::Failed,
                                 Runtime::ExecutionFailureReason::ResourceUnavailable,
                                 0, {"Transaction not found: " + transactionId});
    }

    const uint64_t now = m_clock->NowMs();
    auto& rec = it->second;
    rec.context.currentState      = TransactionState::Aborted;
    rec.context.abortedAtMs       = now;
    rec.context.rollbackRequested = rec.policy.rollbackOnFailure;

    uint32_t duration = (rec.context.startedAtMs > 0 && now >= rec.context.startedAtMs) ?
                        static_cast<uint32_t>(now - rec.context.startedAtMs) : 0;

    m_stats.RecordAborted();
    m_stats.RecordLifetime(duration);

    return TransactionResult(false, TransactionState::Aborted,
                             Runtime::ExecutionFailureReason::ExecutionRejected,
                             duration, {"Transaction aborted: " + reason});
}

std::optional<ExecutionTransaction> TransactionManager::GetTransaction(const TransactionId& transactionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transactions.find(transactionId);
    if (it == m_transactions.end()) return std::nullopt;
    return it->second.transaction;
}

std::optional<ExecutionTransactionContext> TransactionManager::GetContext(const TransactionId& transactionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transactions.find(transactionId);
    if (it == m_transactions.end()) return std::nullopt;
    return it->second.context;
}

bool TransactionManager::UpdateContext(const TransactionId& transactionId, const ExecutionTransactionContext& context) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transactions.find(transactionId);
    if (it == m_transactions.end()) return false;
    it->second.context = context;
    return true;
}

void TransactionManager::CloseTransaction(const TransactionId& transactionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transactions.find(transactionId);
    if (it != m_transactions.end()) {
        m_transactions.erase(it);
    }
}

void TransactionManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (std::size_t i = 0; i < m_transactions.size(); ++i) {
        m_stats.RecordAborted();
    }
    m_transactions.clear();
}

} // namespace Transaction
} // namespace NetDiscovery
