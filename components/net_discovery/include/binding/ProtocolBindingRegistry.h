/**
 * @file ProtocolBindingRegistry.h
 * @brief Exclusive authoritative registry managing ActionBindings, ProtocolAdapterDescriptors, and AdapterRuntimeStates (v5.0.0 Architecture Phase 8.1).
 * 
 * Provides O(1) and O(k) indexed queries, hot replacement, and minimal lock-lifetime snapshotting.
 */

#pragma once

#include "binding/ActionBinding.h"
#include "binding/ProtocolAdapterDescriptor.h"
#include "binding/AdapterRuntimeState.h"
#include "binding/ProtocolBindingRegistrySnapshot.h"
#include "core/StorageEventBus.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <mutex>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Single authoritative owner of binding metadata, adapter descriptors, and runtime states.
 */
class ProtocolBindingRegistry {
public:
    explicit ProtocolBindingRegistry(StorageEventBus* eventBus = nullptr);
    ~ProtocolBindingRegistry() = default;

    ProtocolBindingRegistry(const ProtocolBindingRegistry&) = delete;
    ProtocolBindingRegistry& operator=(const ProtocolBindingRegistry&) = delete;

    void SetEventBus(StorageEventBus* eventBus);

    // ── Snapshot API (Zero lock-contention point-in-time copy) ─────────────
    ProtocolBindingRegistrySnapshot GetSnapshot() const;

    // ── ActionBinding Management ───────────────────────────────────────────
    bool RegisterBinding(const ActionBinding& binding);
    bool ReplaceBinding(const ActionBinding& binding);
    bool RemoveBinding(const BindingId& bindingId);

    // ── ProtocolAdapterDescriptor Management ──────────────────────────────
    bool RegisterAdapter(const ProtocolAdapterDescriptor& adapter);
    bool ReplaceAdapterDescriptor(const ProtocolAdapterDescriptor& adapter);
    bool RemoveAdapter(const AdapterId& adapterId);

    // ── AdapterRuntimeState Management ────────────────────────────────────
    bool UpdateAdapterRuntimeState(const AdapterRuntimeState& state);
    bool UpdateAdapterHealth(const AdapterId& adapterId, AdapterHealthState health, AdapterAvailability availability);
    std::optional<AdapterRuntimeState> GetAdapterRuntimeState(const AdapterId& adapterId) const;

    // ── Direct Query Methods ───────────────────────────────────────────────
    std::optional<ActionBinding> FindBinding(const BindingId& bindingId) const;
    std::optional<ProtocolAdapterDescriptor> FindAdapter(const AdapterId& adapterId) const;

    std::vector<ActionBinding> GetBindingsForOperation(const OperationId& opId) const;
    std::vector<ActionBinding> GetBindingsForAdapter(const AdapterId& adapterId) const;
    std::vector<ActionBinding> GetBindingsForProtocol(const std::string& protocol) const;
    std::vector<ActionBinding> GetBindingsForCapability(const std::string& capabilityId) const;
    std::vector<ActionBinding> GetBindingsForTransport(const std::string& transport) const;

    std::vector<ActionBinding> GetAllBindings() const;
    std::vector<ProtocolAdapterDescriptor> GetAllAdapters() const;
    std::vector<AdapterRuntimeState> GetAllRuntimeStates() const;

    size_t GetBindingCount() const;
    size_t GetAdapterCount() const;
    void Clear();

private:
    void PublishEvent(StorageEventType type, const std::string& bindingId, const std::string& adapterId, const std::string& opId);
    void RemoveFromIndexesInternal(const ActionBinding& binding);
    void AddToIndexesInternal(const ActionBinding& binding);

    mutable std::mutex m_mutex;
    StorageEventBus* m_eventBus{nullptr};

    // Authoritative Primary Tables
    std::unordered_map<BindingId, ActionBinding> m_bindingsById;
    std::unordered_map<AdapterId, ProtocolAdapterDescriptor> m_adaptersById;
    std::unordered_map<AdapterId, AdapterRuntimeState> m_runtimeStatesById;

    // Secondary Hash Indexes
    std::unordered_multimap<OperationId, BindingId> m_bindingsByOperation;
    std::unordered_multimap<AdapterId, BindingId> m_bindingsByAdapter;
    std::unordered_multimap<std::string, BindingId> m_bindingsByProtocol;
};

} // namespace Binding
} // namespace NetDiscovery
