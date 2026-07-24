/**
 * @file TransactionResult.h
 * @brief Immutable transaction result outcome object (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include "transaction/TransactionState.h"
#include "runtime/ExecutionFailureReason.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Transaction {

/**
 * @brief Immutable outcome value object of a transaction lifecycle.
 */
struct TransactionResult {
    bool                            success{false};
    TransactionState                finalState{TransactionState::Created};
    Runtime::ExecutionFailureReason failureReason{Runtime::ExecutionFailureReason::None};
    uint32_t                        durationMs{0};
    std::vector<std::string>        diagnostics;
    std::unordered_map<std::string, std::string> metadata;

    TransactionResult() = default;

    TransactionResult(bool isSuccess,
                      TransactionState state,
                      Runtime::ExecutionFailureReason reason = Runtime::ExecutionFailureReason::None,
                      uint32_t duration = 0,
                      std::vector<std::string> diag = {},
                      std::unordered_map<std::string, std::string> meta = {})
        : success(isSuccess)
        , finalState(state)
        , failureReason(reason)
        , durationMs(duration)
        , diagnostics(std::move(diag))
        , metadata(std::move(meta)) {}

    bool IsSuccess() const { return success && finalState == TransactionState::Committed; }
};

} // namespace Transaction
} // namespace NetDiscovery
