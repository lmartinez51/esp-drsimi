/**
 * @file ProtocolBindingRegistry.cpp
 * @brief Thread-safe indexed implementation of ProtocolBindingRegistry (v5.0.0 Architecture Phase 8.1).
 */

#include "binding/ProtocolBindingRegistry.h"
#include "esp_timer.h"

namespace NetDiscovery {
namespace Binding {

ProtocolBindingRegistry::ProtocolBindingRegistry(StorageEventBus* eventBus)
    : m_eventBus(eventBus) {}

void ProtocolBindingRegistry::SetEventBus(StorageEventBus* eventBus) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventBus = eventBus;
}

ProtocolBindingRegistrySnapshot ProtocolBindingRegistry::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ActionBinding> bList;
    bList.reserve(m_bindingsById.size());
    for (const auto& [bId, binding] : m_bindingsById) {
        bList.push_back(binding);
    }

    std::vector<ProtocolAdapterDescriptor> aList;
    aList.reserve(m_adaptersById.size());
    for (const auto& [aId, adapter] : m_adaptersById) {
        aList.push_back(adapter);
    }

    std::vector<AdapterRuntimeState> rList;
    rList.reserve(m_runtimeStatesById.size());
    for (const auto& [rId, state] : m_runtimeStatesById) {
        rList.push_back(state);
    }

    uint64_t timestamp = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    return ProtocolBindingRegistrySnapshot(std::move(bList), std::move(aList), std::move(rList), timestamp);
}

void ProtocolBindingRegistry::AddToIndexesInternal(const ActionBinding& binding) {
    m_bindingsByOperation.insert({binding.GetOperationId(), binding.GetBindingId()});
    m_bindingsByAdapter.insert({binding.GetAdapterId(), binding.GetBindingId()});
    m_bindingsByProtocol.insert({binding.GetProtocol(), binding.GetBindingId()});
}

void ProtocolBindingRegistry::RemoveFromIndexesInternal(const ActionBinding& binding) {
    auto rangeOp = m_bindingsByOperation.equal_range(binding.GetOperationId());
    for (auto it = rangeOp.first; it != rangeOp.second; ) {
        if (it->second == binding.GetBindingId()) {
            it = m_bindingsByOperation.erase(it);
        } else {
            ++it;
        }
    }

    auto rangeAd = m_bindingsByAdapter.equal_range(binding.GetAdapterId());
    for (auto it = rangeAd.first; it != rangeAd.second; ) {
        if (it->second == binding.GetBindingId()) {
            it = m_bindingsByAdapter.erase(it);
        } else {
            ++it;
        }
    }

    auto rangePr = m_bindingsByProtocol.equal_range(binding.GetProtocol());
    for (auto it = rangePr.first; it != rangePr.second; ) {
        if (it->second == binding.GetBindingId()) {
            it = m_bindingsByProtocol.erase(it);
        } else {
            ++it;
        }
    }
}

bool ProtocolBindingRegistry::RegisterBinding(const ActionBinding& binding) {
    if (binding.GetBindingId().empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_bindingsById.find(binding.GetBindingId()) != m_bindingsById.end()) {
            return false;
        }
        m_bindingsById.insert({binding.GetBindingId(), binding});
        AddToIndexesInternal(binding);
    }

    PublishEvent(StorageEventType::BindingRegistered, binding.GetBindingId(), binding.GetAdapterId(), binding.GetOperationId());
    return true;
}

bool ProtocolBindingRegistry::ReplaceBinding(const ActionBinding& binding) {
    if (binding.GetBindingId().empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_bindingsById.find(binding.GetBindingId());
        if (it != m_bindingsById.end()) {
            RemoveFromIndexesInternal(it->second);
            it->second = binding;
            AddToIndexesInternal(binding);
        } else {
            m_bindingsById.insert({binding.GetBindingId(), binding});
            AddToIndexesInternal(binding);
        }
    }

    PublishEvent(StorageEventType::BindingUpdated, binding.GetBindingId(), binding.GetAdapterId(), binding.GetOperationId());
    return true;
}

bool ProtocolBindingRegistry::RemoveBinding(const BindingId& bindingId) {
    if (bindingId.empty()) return false;

    std::string adapterId;
    std::string opId;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_bindingsById.find(bindingId);
        if (it == m_bindingsById.end()) return false;

        adapterId = it->second.GetAdapterId();
        opId = it->second.GetOperationId();
        RemoveFromIndexesInternal(it->second);
        m_bindingsById.erase(it);
    }

    PublishEvent(StorageEventType::BindingRemoved, bindingId, adapterId, opId);
    return true;
}

bool ProtocolBindingRegistry::RegisterAdapter(const ProtocolAdapterDescriptor& adapter) {
    if (adapter.adapterId.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_adaptersById.find(adapter.adapterId) != m_adaptersById.end()) {
            return false;
        }
        m_adaptersById.insert({adapter.adapterId, adapter});
        
        // Initialize default runtime state if not present
        if (m_runtimeStatesById.find(adapter.adapterId) == m_runtimeStatesById.end()) {
            m_runtimeStatesById.insert({adapter.adapterId, AdapterRuntimeState(adapter.adapterId)});
        }
    }

    PublishEvent(StorageEventType::AdapterRegistered, "", adapter.adapterId, "");
    return true;
}

bool ProtocolBindingRegistry::ReplaceAdapterDescriptor(const ProtocolAdapterDescriptor& adapter) {
    if (adapter.adapterId.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_adaptersById[adapter.adapterId] = adapter;
        if (m_runtimeStatesById.find(adapter.adapterId) == m_runtimeStatesById.end()) {
            m_runtimeStatesById.insert({adapter.adapterId, AdapterRuntimeState(adapter.adapterId)});
        }
    }

    PublishEvent(StorageEventType::AdapterUpdated, "", adapter.adapterId, "");
    return true;
}

bool ProtocolBindingRegistry::RemoveAdapter(const AdapterId& adapterId) {
    if (adapterId.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_adaptersById.erase(adapterId);
        m_runtimeStatesById.erase(adapterId);
    }

    PublishEvent(StorageEventType::BindingRemoved, "", adapterId, "");
    return true;
}

bool ProtocolBindingRegistry::UpdateAdapterRuntimeState(const AdapterRuntimeState& state) {
    if (state.adapterId.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_runtimeStatesById.find(state.adapterId);
        if (it != m_runtimeStatesById.end()) {
            it->second = state;
        } else {
            m_runtimeStatesById.insert({state.adapterId, state});
        }
    }

    PublishEvent(StorageEventType::AdapterHealthChanged, "", state.adapterId, "");
    return true;
}

bool ProtocolBindingRegistry::UpdateAdapterHealth(const AdapterId& adapterId, AdapterHealthState health, AdapterAvailability availability) {
    if (adapterId.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_runtimeStatesById.find(adapterId);
        if (it != m_runtimeStatesById.end()) {
            it->second.TransitionState(health, availability);
        } else {
            AdapterRuntimeState newState(adapterId);
            newState.TransitionState(health, availability);
            m_runtimeStatesById.insert({adapterId, newState});
        }
    }

    PublishEvent(StorageEventType::AdapterHealthChanged, "", adapterId, "");
    return true;
}

std::optional<AdapterRuntimeState> ProtocolBindingRegistry::GetAdapterRuntimeState(const AdapterId& adapterId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_runtimeStatesById.find(adapterId);
    if (it != m_runtimeStatesById.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ActionBinding> ProtocolBindingRegistry::FindBinding(const BindingId& bindingId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_bindingsById.find(bindingId);
    if (it != m_bindingsById.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ProtocolAdapterDescriptor> ProtocolBindingRegistry::FindAdapter(const AdapterId& adapterId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_adaptersById.find(adapterId);
    if (it != m_adaptersById.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<ActionBinding> ProtocolBindingRegistry::GetBindingsForOperation(const OperationId& opId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ActionBinding> result;
    auto range = m_bindingsByOperation.equal_range(opId);
    for (auto it = range.first; it != range.second; ++it) {
        auto bIt = m_bindingsById.find(it->second);
        if (bIt != m_bindingsById.end()) {
            result.push_back(bIt->second);
        }
    }
    return result;
}

std::vector<ActionBinding> ProtocolBindingRegistry::GetBindingsForAdapter(const AdapterId& adapterId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ActionBinding> result;
    auto range = m_bindingsByAdapter.equal_range(adapterId);
    for (auto it = range.first; it != range.second; ++it) {
        auto bIt = m_bindingsById.find(it->second);
        if (bIt != m_bindingsById.end()) {
            result.push_back(bIt->second);
        }
    }
    return result;
}

std::vector<ActionBinding> ProtocolBindingRegistry::GetBindingsForProtocol(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ActionBinding> result;
    auto range = m_bindingsByProtocol.equal_range(protocol);
    for (auto it = range.first; it != range.second; ++it) {
        auto bIt = m_bindingsById.find(it->second);
        if (bIt != m_bindingsById.end()) {
            result.push_back(bIt->second);
        }
    }
    return result;
}

std::vector<ActionBinding> ProtocolBindingRegistry::GetBindingsForCapability(const std::string& capabilityId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ActionBinding> result;
    for (const auto& [bId, binding] : m_bindingsById) {
        auto aIt = m_adaptersById.find(binding.GetAdapterId());
        if (aIt != m_adaptersById.end()) {
            for (const auto& cap : aIt->second.supportedCapabilities) {
                if (cap == capabilityId) {
                    result.push_back(binding);
                    break;
                }
            }
        }
    }
    return result;
}

std::vector<ActionBinding> ProtocolBindingRegistry::GetBindingsForTransport(const std::string& transport) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ActionBinding> result;
    for (const auto& [bId, binding] : m_bindingsById) {
        auto aIt = m_adaptersById.find(binding.GetAdapterId());
        if (aIt != m_adaptersById.end() && aIt->second.transport == transport) {
            result.push_back(binding);
        }
    }
    return result;
}

std::vector<ActionBinding> ProtocolBindingRegistry::GetAllBindings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ActionBinding> result;
    result.reserve(m_bindingsById.size());
    for (const auto& [bId, binding] : m_bindingsById) {
        result.push_back(binding);
    }
    return result;
}

std::vector<ProtocolAdapterDescriptor> ProtocolBindingRegistry::GetAllAdapters() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ProtocolAdapterDescriptor> result;
    result.reserve(m_adaptersById.size());
    for (const auto& [aId, adapter] : m_adaptersById) {
        result.push_back(adapter);
    }
    return result;
}

std::vector<AdapterRuntimeState> ProtocolBindingRegistry::GetAllRuntimeStates() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AdapterRuntimeState> result;
    result.reserve(m_runtimeStatesById.size());
    for (const auto& [rId, state] : m_runtimeStatesById) {
        result.push_back(state);
    }
    return result;
}

size_t ProtocolBindingRegistry::GetBindingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bindingsById.size();
}

size_t ProtocolBindingRegistry::GetAdapterCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_adaptersById.size();
}

void ProtocolBindingRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bindingsById.clear();
    m_adaptersById.clear();
    m_runtimeStatesById.clear();
    m_bindingsByOperation.clear();
    m_bindingsByAdapter.clear();
    m_bindingsByProtocol.clear();
}

void ProtocolBindingRegistry::PublishEvent(StorageEventType type, const std::string& bindingId, const std::string& adapterId, const std::string& opId) {
    if (!m_eventBus) return;

    StorageEvent event;
    event.type = type;
    event.entityId = bindingId;
    event.timestamp = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    if (!bindingId.empty()) event.metadata["BindingId"] = bindingId;
    if (!adapterId.empty()) event.metadata["AdapterId"] = adapterId;
    if (!opId.empty()) event.metadata["OperationId"] = opId;

    m_eventBus->Publish(event);
}

} // namespace Binding
} // namespace NetDiscovery
