/**
 * @file TransactionState.h
 * @brief Strongly typed lifecycle state enum for transactions (v5.0.0 Architecture Phase 14).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Transaction {

/**
 * @brief Strongly typed lifecycle states for an ExecutionTransaction.
 */
enum class TransactionState {
    Created,
    Ready,
    Running,
    Committing,
    Committed,
    Aborting,
    Aborted,
    Failed,
    TimedOut,
    Cancelled
};

/**
 * @brief Converts TransactionState enum to string representation.
 */
inline std::string ToString(TransactionState state) {
    switch (state) {
        case TransactionState::Created:    return "Created";
        case TransactionState::Ready:      return "Ready";
        case TransactionState::Running:    return "Running";
        case TransactionState::Committing: return "Committing";
        case TransactionState::Committed:  return "Committed";
        case TransactionState::Aborting:   return "Aborting";
        case TransactionState::Aborted:    return "Aborted";
        case TransactionState::Failed:     return "Failed";
        case TransactionState::TimedOut:   return "TimedOut";
        case TransactionState::Cancelled:  return "Cancelled";
        default:                           return "Unknown";
    }
}

} // namespace Transaction
} // namespace NetDiscovery
