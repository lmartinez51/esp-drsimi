/**
 * @file ExecutionTransaction.h
 * @brief Immutable identity object representing an execution transaction (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include <string>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Transaction {

using TransactionId = std::string;

/**
 * @brief Immutable identity object representing an execution transaction.
 *
 * Contains ZERO mutable runtime state. Pure value object.
 */
struct ExecutionTransaction {
    TransactionId transactionId;
    std::string   sessionId;
    uint64_t      creationTimestampMs{0};
    uint64_t      generationVersion{1};
    std::string   transactionPolicyId;

    ExecutionTransaction() = default;

    ExecutionTransaction(TransactionId tid,
                         std::string sid,
                         uint64_t created = 0,
                         uint64_t gen = 1,
                         std::string policyId = "default")
        : transactionId(std::move(tid))
        , sessionId(std::move(sid))
        , creationTimestampMs(created)
        , generationVersion(gen)
        , transactionPolicyId(std::move(policyId)) {}

    bool IsValid() const { return !transactionId.empty() && !sessionId.empty(); }
};

} // namespace Transaction
} // namespace NetDiscovery
