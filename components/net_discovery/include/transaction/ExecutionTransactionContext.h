/**
 * @file ExecutionTransactionContext.h
 * @brief Mutable runtime state for an execution transaction (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include "transaction/ExecutionTransaction.h"
#include "transaction/TransactionState.h"
#include "execution/ExecutionPlannerTypes.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Transaction {

/**
 * @brief Mutable runtime context owning step completion lists and transaction state.
 */
struct ExecutionTransactionContext {
    TransactionId                  transactionId;
    TransactionState               currentState{TransactionState::Created};
    uint64_t                       startedAtMs{0};
    uint64_t                       committedAtMs{0};
    uint64_t                       abortedAtMs{0};
    bool                           rollbackRequested{false};
    std::vector<Execution::StepId> completedSteps;
    std::vector<Execution::StepId> failedSteps;
    std::vector<Execution::StepId> pendingSteps;
    std::unordered_map<std::string, std::string> transactionVariables;
    std::unordered_map<std::string, std::string> metadata;
    std::vector<std::string>       diagnostics;

    ExecutionTransactionContext() = default;

    explicit ExecutionTransactionContext(TransactionId id)
        : transactionId(std::move(id)) {}

    void RecordStepCompleted(const Execution::StepId& stepId) {
        completedSteps.push_back(stepId);
    }

    void RecordStepFailed(const Execution::StepId& stepId) {
        failedSteps.push_back(stepId);
    }
};

} // namespace Transaction
} // namespace NetDiscovery
