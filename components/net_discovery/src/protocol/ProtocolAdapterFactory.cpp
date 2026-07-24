/**
 * @file ProtocolAdapterFactory.cpp
 * @brief Implementation of ProtocolAdapterFactory (v5.0.0 Architecture Phase 10).
 */

#include "protocol/ProtocolAdapterFactory.h"

namespace NetDiscovery {
namespace Protocol {

void ProtocolAdapterFactory::RegisterCreator(const std::string& protocolName,
                                              AdapterCreator creator) {
    if (protocolName.empty() || !creator) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_creators.insert_or_assign(protocolName, std::move(creator));
}

bool ProtocolAdapterFactory::UnregisterCreator(const std::string& protocolName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_creators.erase(protocolName) > 0;
}

bool ProtocolAdapterFactory::HasCreator(const std::string& protocolName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_creators.count(protocolName) > 0;
}

std::shared_ptr<IProtocolAdapter>
ProtocolAdapterFactory::Create(const std::string& protocolName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_creators.find(protocolName);
    if (it == m_creators.end()) return nullptr;
    return it->second();  // invoke creator
}

std::vector<std::string> ProtocolAdapterFactory::GetRegisteredProtocols() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    names.reserve(m_creators.size());
    for (const auto& kv : m_creators) {
        names.push_back(kv.first);
    }
    return names;
}

} // namespace Protocol
} // namespace NetDiscovery
