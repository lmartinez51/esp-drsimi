/**
 * @file TransactionManager.h
 * @brief Reusable, thread-safe implementation of ITransactionManager (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include "transaction/ITransactionManager.h"
#include "runtime/ExecutionClock.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace NetDiscovery {
namespace Transaction {

/**
 * @brief Thread-safe implementation of ITransactionManager.
 *
 * Owns transaction creation, context updates, commit, abort, and telemetry tracking.
 * Contains ZERO protocol code, ZERO networking, and ZERO socket logic.
 */
class TransactionManager : public ITransactionManager {
public:
    explicit TransactionManager(uint32_t maxTransactions = 32,
                                std::shared_ptr<Runtime::IExecutionClock> clock = nullptr);

    ~TransactionManager() override = default;

    // ── ITransactionManager ─────────────────────────────────────────────────

    std::optional<ExecutionTransaction> BeginTransaction(
        const std::string& sessionId,
        const TransactionPolicy& policy = TransactionPolicy()) override;

    TransactionResult Commit(const TransactionId& transactionId) override;
    TransactionResult Abort(const TransactionId& transactionId, const std::string& reason = "") override;

    std::optional<ExecutionTransaction> GetTransaction(const TransactionId& transactionId) const override;
    std::optional<ExecutionTransactionContext> GetContext(const TransactionId& transactionId) const override;
    bool UpdateContext(const TransactionId& transactionId, const ExecutionTransactionContext& context) override;

    void CloseTransaction(const TransactionId& transactionId) override;
    void Shutdown() override;

    const TransactionStatistics& GetStatistics() const override { return m_stats; }

private:
    struct TransactionRecord {
        ExecutionTransaction        transaction;
        ExecutionTransactionContext context;
        TransactionPolicy           policy;
    };

    uint32_t m_maxTransactions{32};
    std::shared_ptr<Runtime::IExecutionClock> m_clock;

    TransactionStatistics m_stats;

    mutable std::mutex m_mutex;
    std::unordered_map<TransactionId, TransactionRecord> m_transactions;
    uint64_t m_nextId{1};
};

} // namespace Transaction
} // namespace NetDiscovery
