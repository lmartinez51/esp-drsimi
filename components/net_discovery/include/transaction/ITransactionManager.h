/**
 * @file ITransactionManager.h
 * @brief Pure abstract interface for transaction management (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include "transaction/ExecutionTransaction.h"
#include "transaction/ExecutionTransactionContext.h"
#include "transaction/TransactionPolicy.h"
#include "transaction/TransactionResult.h"
#include "transaction/TransactionStatistics.h"

#include <optional>
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Transaction {

/**
 * @brief Pure abstract interface for managing execution transaction lifecycles.
 */
class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;

    /**
     * @brief Begins a new ExecutionTransaction for a given session.
     */
    virtual std::optional<ExecutionTransaction> BeginTransaction(
        const std::string& sessionId,
        const TransactionPolicy& policy = TransactionPolicy()) = 0;

    /**
     * @brief Commits an active transaction.
     */
    virtual TransactionResult Commit(const TransactionId& transactionId) = 0;

    /**
     * @brief Aborts an active transaction with a diagnostic reason.
     */
    virtual TransactionResult Abort(const TransactionId& transactionId, const std::string& reason = "") = 0;

    /**
     * @brief Returns an immutable ExecutionTransaction by ID.
     */
    virtual std::optional<ExecutionTransaction> GetTransaction(const TransactionId& transactionId) const = 0;

    /**
     * @brief Returns mutable ExecutionTransactionContext by ID.
     */
    virtual std::optional<ExecutionTransactionContext> GetContext(const TransactionId& transactionId) const = 0;

    /**
     * @brief Updates transaction context for an active transaction.
     */
    virtual bool UpdateContext(const TransactionId& transactionId, const ExecutionTransactionContext& context) = 0;

    /**
     * @brief Closes and destroys a transaction.
     */
    virtual void CloseTransaction(const TransactionId& transactionId) = 0;

    /**
     * @brief Closes all active transactions and resets manager state.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Returns telemetry statistics.
     */
    virtual const TransactionStatistics& GetStatistics() const = 0;
};

} // namespace Transaction
} // namespace NetDiscovery
