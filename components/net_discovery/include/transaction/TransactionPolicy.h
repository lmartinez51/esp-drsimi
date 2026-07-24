/**
 * @file TransactionPolicy.h
 * @brief Immutable transaction policy metadata (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Transaction {

/**
 * @brief Immutable transaction policy defining execution behavior rules.
 */
struct TransactionPolicy {
    std::string policyId{"default"};
    bool        allowPartialCommit{false};
    bool        rollbackOnFailure{true};
    uint32_t    maximumRetries{0};
    uint32_t    timeoutMs{30000};
    std::string isolationLevel{"ReadCommitted"};
    std::string nestedTransactionPolicy{"Prohibit"};
    std::unordered_map<std::string, std::string> metadata;

    TransactionPolicy() = default;

    TransactionPolicy(std::string id,
                      bool partial = false,
                      bool rollback = true,
                      uint32_t retries = 0,
                      uint32_t timeout = 30000,
                      std::string iso = "ReadCommitted",
                      std::string nested = "Prohibit")
        : policyId(std::move(id))
        , allowPartialCommit(partial)
        , rollbackOnFailure(rollback)
        , maximumRetries(retries)
        , timeoutMs(timeout)
        , isolationLevel(std::move(iso))
        , nestedTransactionPolicy(std::move(nested)) {}
};

} // namespace Transaction
} // namespace NetDiscovery
