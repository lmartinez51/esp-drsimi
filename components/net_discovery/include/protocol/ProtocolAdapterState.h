/**
 * @file ProtocolAdapterState.h
 * @brief Mutable runtime state for a registered protocol adapter (v5.0.0 Architecture Phase 10).
 */

#pragma once

#include "protocol/ProtocolAdapterDescriptor.h"

#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Lifecycle state of a protocol adapter at runtime.
 */
enum class AdapterLifecycleState {
    Uninitialized,   ///< Registered but Initialize() not called.
    Initializing,    ///< Initialize() in progress.
    Initialized,     ///< Initialize() completed successfully.
    Available,       ///< Ready to accept step dispatch.
    Busy,            ///< Actively executing a step.
    Offline,         ///< Temporarily unreachable (will retry).
    Error,           ///< Fatal error state; manual recovery required.
    ShuttingDown,    ///< Shutdown() in progress.
    Shutdown         ///< Shutdown() completed; adapter no longer usable.
};

/**
 * @brief Converts AdapterLifecycleState to a human-readable string.
 */
inline std::string ToString(AdapterLifecycleState state) {
    switch (state) {
        case AdapterLifecycleState::Uninitialized: return "Uninitialized";
        case AdapterLifecycleState::Initializing:  return "Initializing";
        case AdapterLifecycleState::Initialized:   return "Initialized";
        case AdapterLifecycleState::Available:     return "Available";
        case AdapterLifecycleState::Busy:          return "Busy";
        case AdapterLifecycleState::Offline:       return "Offline";
        case AdapterLifecycleState::Error:         return "Error";
        case AdapterLifecycleState::ShuttingDown:  return "ShuttingDown";
        case AdapterLifecycleState::Shutdown:      return "Shutdown";
        default:                                   return "Unknown";
    }
}

/**
 * @brief Mutable runtime snapshot of an adapter's current operational status.
 *
 * Owned by AdapterLifecycleManager. Never stored in ProtocolAdapterDescriptor.
 * Updated exclusively by AdapterLifecycleManager during lifecycle transitions.
 */
struct ProtocolAdapterState {
    AdapterId              adapterId;
    AdapterLifecycleState  lifecycleState{AdapterLifecycleState::Uninitialized};
    std::string            lastError;           ///< Last error message; empty if no error.
    uint64_t               lastHeartbeatMs{0};  ///< Timestamp of last successful health check (ms).
    uint32_t               latencyMs{0};        ///< Last measured round-trip latency (ms).
    uint64_t               stateVersion{0};     ///< Monotonically increasing version counter for state changes.

    ProtocolAdapterState() = default;
    explicit ProtocolAdapterState(AdapterId id)
        : adapterId(std::move(id)) {}

    bool IsAvailable()    const { return lifecycleState == AdapterLifecycleState::Available; }
    bool IsInitialized()  const { return lifecycleState == AdapterLifecycleState::Initialized ||
                                         lifecycleState == AdapterLifecycleState::Available  ||
                                         lifecycleState == AdapterLifecycleState::Busy; }
    bool IsTerminal()     const { return lifecycleState == AdapterLifecycleState::Shutdown ||
                                         lifecycleState == AdapterLifecycleState::Error; }

    void BumpVersion() { ++stateVersion; }
};

} // namespace Protocol
} // namespace NetDiscovery
