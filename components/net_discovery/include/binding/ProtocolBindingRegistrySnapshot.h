/**
 * @file ProtocolBindingRegistrySnapshot.h
 * @brief Immutable point-in-time snapshot container for ProtocolBindingRegistry (v5.0.0 Architecture Phase 8.1).
 * 
 * Contains complete copies of registered ActionBindings, ProtocolAdapterDescriptors, and 
 * AdapterRuntimeStates. Completely free of mutexes, pointers, or lazy loading. Safe to consume 
 * concurrently by UI, AI reasoning, telemetry, and diagnostic components.
 */

#pragma once

#include "binding/ActionBinding.h"
#include "binding/ProtocolAdapterDescriptor.h"
#include "binding/AdapterRuntimeState.h"

#include <vector>
#include <cstdint>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Point-in-time immutable snapshot of the entire binding registry.
 */
struct ProtocolBindingRegistrySnapshot {
    std::vector<ActionBinding> bindings;               // Full point-in-time copy of bindings
    std::vector<ProtocolAdapterDescriptor> adapters;   // Full point-in-time copy of immutable adapter descriptors
    std::vector<AdapterRuntimeState> runtimeStates;     // Full point-in-time copy of dynamic adapter runtime states
    uint64_t snapshotTimestampMs{0};                    // Monotonic time at snapshot creation

    ProtocolBindingRegistrySnapshot() = default;

    ProtocolBindingRegistrySnapshot(std::vector<ActionBinding> b,
                                   std::vector<ProtocolAdapterDescriptor> a,
                                   std::vector<AdapterRuntimeState> r,
                                   uint64_t timestamp = 0)
        : bindings(std::move(b)),
          adapters(std::move(a)),
          runtimeStates(std::move(r)),
          snapshotTimestampMs(timestamp) {}

    size_t GetBindingCount() const { return bindings.size(); }
    size_t GetAdapterCount() const { return adapters.size(); }
};

} // namespace Binding
} // namespace NetDiscovery
