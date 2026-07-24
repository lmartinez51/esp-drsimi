/**
 * @file ExecutionFailureReason.h
 * @brief Independent failure taxonomy enum for execution step outcomes (v5.0.0 Architecture Phase 13.1).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Protocol-independent taxonomy answering why an execution attempt failed.
 *
 * Strictly orthogonal to Execution::StepStatus (which answers whether execution succeeded).
 */
enum class ExecutionFailureReason {
    None,
    CapabilityMismatch,
    ValidationFailure,
    AuthenticationFailure,
    AuthorizationFailure,
    TransportFailure,
    ProtocolFailure,
    Timeout,
    Cancelled,
    RetryLimitExceeded,
    ResourceUnavailable,
    DeviceUnavailable,
    DependencyFailure,
    ExecutionRejected,
    InternalError,
    Unknown
};

/**
 * @brief Converts ExecutionFailureReason enum to string representation.
 */
inline std::string ToString(ExecutionFailureReason reason) {
    switch (reason) {
        case ExecutionFailureReason::None:                  return "None";
        case ExecutionFailureReason::CapabilityMismatch:    return "CapabilityMismatch";
        case ExecutionFailureReason::ValidationFailure:     return "ValidationFailure";
        case ExecutionFailureReason::AuthenticationFailure: return "AuthenticationFailure";
        case ExecutionFailureReason::AuthorizationFailure:  return "AuthorizationFailure";
        case ExecutionFailureReason::TransportFailure:      return "TransportFailure";
        case ExecutionFailureReason::ProtocolFailure:       return "ProtocolFailure";
        case ExecutionFailureReason::Timeout:               return "Timeout";
        case ExecutionFailureReason::Cancelled:             return "Cancelled";
        case ExecutionFailureReason::RetryLimitExceeded:    return "RetryLimitExceeded";
        case ExecutionFailureReason::ResourceUnavailable:   return "ResourceUnavailable";
        case ExecutionFailureReason::DeviceUnavailable:     return "DeviceUnavailable";
        case ExecutionFailureReason::DependencyFailure:     return "DependencyFailure";
        case ExecutionFailureReason::ExecutionRejected:     return "ExecutionRejected";
        case ExecutionFailureReason::InternalError:         return "InternalError";
        case ExecutionFailureReason::Unknown:               return "Unknown";
        default:                                            return "Unknown";
    }
}

} // namespace Runtime
} // namespace NetDiscovery
