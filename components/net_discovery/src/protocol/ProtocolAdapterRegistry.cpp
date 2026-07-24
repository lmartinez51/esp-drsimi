/**
 * @file ProtocolAdapterRegistry.cpp
 * @brief Implementation of ProtocolAdapterRegistry (v5.0.0 Architecture Phase 10).
 */

#include "protocol/ProtocolAdapterRegistry.h"

#include <algorithm>

namespace NetDiscovery {
namespace Protocol {

void ProtocolAdapterRegistry::Register(std::shared_ptr<IProtocolAdapter> adapter) {
    if (!adapter) return;
    const AdapterId id = adapter->GetDescriptor().adapterId;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_adapters.insert_or_assign(id, std::move(adapter));
}

bool ProtocolAdapterRegistry::Remove(const AdapterId& adapterId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_adapters.erase(adapterId) > 0;
}

std::shared_ptr<IProtocolAdapter> ProtocolAdapterRegistry::Find(const AdapterId& adapterId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_adapters.find(adapterId);
    return it != m_adapters.end() ? it->second : nullptr;
}

bool ProtocolAdapterRegistry::Contains(const AdapterId& adapterId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_adapters.count(adapterId) > 0;
}

std::size_t ProtocolAdapterRegistry::Count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_adapters.size();
}

std::vector<std::shared_ptr<IProtocolAdapter>>
ProtocolAdapterRegistry::FindByProtocol(const std::string& protocolName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<IProtocolAdapter>> result;
    for (const auto& kv : m_adapters) {
        if (kv.second->GetDescriptor().protocolName == protocolName) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<std::shared_ptr<IProtocolAdapter>>
ProtocolAdapterRegistry::FindByOperation(const std::string& operationId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<IProtocolAdapter>> result;
    for (const auto& kv : m_adapters) {
        if (kv.second->GetDescriptor().SupportsOperation(operationId)) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<std::shared_ptr<IProtocolAdapter>>
ProtocolAdapterRegistry::FindByCapability(const std::string& capability) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<IProtocolAdapter>> result;
    for (const auto& kv : m_adapters) {
        if (kv.second->GetDescriptor().SupportsCapability(capability)) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<std::shared_ptr<IProtocolAdapter>>
ProtocolAdapterRegistry::GetAvailableAdapters() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<IProtocolAdapter>> result;
    for (const auto& kv : m_adapters) {
        if (kv.second->IsAvailable()) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<AdapterId> ProtocolAdapterRegistry::GetAllAdapterIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AdapterId> ids;
    ids.reserve(m_adapters.size());
    for (const auto& kv : m_adapters) {
        ids.push_back(kv.first);
    }
    return ids;
}

std::vector<ProtocolAdapterDescriptor> ProtocolAdapterRegistry::GetAllDescriptors() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ProtocolAdapterDescriptor> descriptors;
    descriptors.reserve(m_adapters.size());
    for (const auto& kv : m_adapters) {
        descriptors.push_back(kv.second->GetDescriptor());
    }
    return descriptors;
}

} // namespace Protocol
} // namespace NetDiscovery
