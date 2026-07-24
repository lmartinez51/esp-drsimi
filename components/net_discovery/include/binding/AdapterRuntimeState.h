/**
 * @file AdapterRuntimeState.h
 * @brief Dynamic mutable runtime state container for protocol adapters (v5.0.0 Architecture Phase 8.1).
 * 
 * Separates volatile dynamic metrics (health, availability, stateVersion, latencies, errors) 
 * from the static immutable ProtocolAdapterDescriptor architecture.
 */

#pragma once

#include "binding/ProtocolAdapterDescriptor.h"

#include <string>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Dynamic runtime execution state for a registered protocol adapter.
 */
struct AdapterRuntimeState {
    AdapterId adapterId;                                                // Associated adapter identifier
    AdapterHealthState healthState{AdapterHealthState::Healthy};         // Dynamic health status
    AdapterAvailability availability{AdapterAvailability::Available};   // Dynamic availability status
    bool authenticated{false};                                          // Auth token/key readiness
    uint64_t stateVersion{1};                                           // Incremental state transition counter
    uint64_t lastHeartbeatMs{0};                                        // Timestamp of last heartbeat
    uint64_t lastSuccessMs{0};                                          // Timestamp of last successful execution
    uint64_t lastFailureMs{0};                                          // Timestamp of last failed execution
    std::optional<std::string> lastError;                               // Last error message snippet
    uint32_t averageLatencyMs{0};                                       // Rolling average execution latency in ms
    std::unordered_map<std::string, std::string> metadata;             // Dynamic runtime metadata

    AdapterRuntimeState() = default;
    
    explicit AdapterRuntimeState(AdapterId id)
        : adapterId(std::move(id)) {}

    /**
     * @brief Transitions runtime health and availability while automatically incrementing stateVersion.
     */
    void TransitionState(AdapterHealthState newHealth, AdapterAvailability newAvailability) {
        if (healthState != newHealth || availability != newAvailability) {
            healthState = newHealth;
            availability = newAvailability;
            stateVersion++;
        }
    }

    bool IsHealthy() const {
        return healthState == AdapterHealthState::Healthy || healthState == AdapterHealthState::Degraded;
    }

    bool IsAvailable() const {
        return availability == AdapterAvailability::Available;
    }

    bool operator==(const AdapterRuntimeState& other) const {
        return adapterId == other.adapterId && stateVersion == other.stateVersion;
    }
};

} // namespace Binding
} // namespace NetDiscovery
