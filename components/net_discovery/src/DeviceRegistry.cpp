#include "../include/DeviceRegistry.h"
#include <algorithm>

namespace NetDiscovery {

void DeviceRegistry::Register(const IdentityEvidence& evidence)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Basic deduplication: if we already have an evidence from the same source
    // for the same UUID with the same IP, don't duplicate it.
    auto it = std::find_if(m_evidence.begin(), m_evidence.end(), [&](const IdentityEvidence& e) {
        return e.source == evidence.source && e.uuid == evidence.uuid && e.ip == evidence.ip;
    });

    if (it == m_evidence.end()) {
        m_evidence.push_back(evidence);
    } else {
        // Just update existing if it's the exact same packet deduplication target
        *it = evidence;
    }
}

void DeviceRegistry::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_evidence.clear();
}

std::vector<IdentityEvidence> DeviceRegistry::GetAll() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_evidence;
}

std::vector<IdentityEvidence> DeviceRegistry::GetEvidencePendingDescription() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IdentityEvidence> result;
    for (const auto& dev : m_evidence) {
        if (dev.protocolEvidence.upnp.has_value() && 
            !dev.protocolEvidence.upnp->locationUrl.empty() && 
            dev.friendlyName.empty()) {
            result.push_back(dev);
        }
    }
    return result;
}

std::size_t DeviceRegistry::Size() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_evidence.size();
}

} // namespace NetDiscovery
