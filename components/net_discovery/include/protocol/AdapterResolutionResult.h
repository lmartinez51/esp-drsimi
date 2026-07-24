/**
 * @file AdapterResolutionResult.h
 * @brief Outcome model returned by IAdapterResolver (v5.0.0 Architecture Phase 10.1).
 */

#pragma once

#include "protocol/ResolvedAdapter.h"

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Resolution status code returned by AdapterResolver.
 */
enum class ResolutionStatus {
    Resolved,               ///< Adapter successfully resolved and ready for execution.
    AdapterNotFound,        ///< Adapter ID is not registered in the registry.
    Unavailable,            ///< Adapter is offline or uninitialized.
    Initializing,           ///< Adapter initialization is currently in progress.
    Busy,                   ///< Adapter is busy processing another operation.
    CapabilityMismatch,     ///< Adapter does not support the operation or required capabilities.
    LifecycleError,         ///< Adapter is in an error state.
    AuthenticationRequired, ///< Adapter requires credential configuration.
    Unknown                 ///< Unmapped resolution failure.
};

/**
 * @brief Converts ResolutionStatus to string.
 */
inline std::string ToString(ResolutionStatus status) {
    switch (status) {
        case ResolutionStatus::Resolved:               return "Resolved";
        case ResolutionStatus::AdapterNotFound:        return "AdapterNotFound";
        case ResolutionStatus::Unavailable:            return "Unavailable";
        case ResolutionStatus::Initializing:           return "Initializing";
        case ResolutionStatus::Busy:                   return "Busy";
        case ResolutionStatus::CapabilityMismatch:     return "CapabilityMismatch";
        case ResolutionStatus::LifecycleError:         return "LifecycleError";
        case ResolutionStatus::AuthenticationRequired: return "AuthenticationRequired";
        default:                                       return "Unknown";
    }
}

/**
 * @brief Outcome container returned by IAdapterResolver.
 */
struct AdapterResolutionResult {
    ResolutionStatus                           status{ResolutionStatus::Unknown};
    std::optional<ResolvedAdapter>              resolvedAdapter;
    std::vector<std::string>                   diagnostics;
    std::string                                failureReason;
    std::unordered_map<std::string, std::string> metadata;

    AdapterResolutionResult() = default;

    AdapterResolutionResult(ResolutionStatus stat,
                            std::optional<ResolvedAdapter> adapter = std::nullopt,
                            std::vector<std::string> diag = {},
                            std::string reason = "",
                            std::unordered_map<std::string, std::string> meta = {})
        : status(stat)
        , resolvedAdapter(std::move(adapter))
        , diagnostics(std::move(diag))
        , failureReason(std::move(reason))
        , metadata(std::move(meta)) {}

    bool IsResolved() const { return status == ResolutionStatus::Resolved && resolvedAdapter.has_value() && resolvedAdapter->IsValid(); }
    bool HasAdapter() const { return resolvedAdapter.has_value() && resolvedAdapter->IsValid(); }
};

} // namespace Protocol
} // namespace NetDiscovery
