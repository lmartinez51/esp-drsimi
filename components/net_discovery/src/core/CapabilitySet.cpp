/**
 * @file CapabilitySet.cpp
 * @brief Lock-free implementation of CapabilitySet value object container (v5.0.0 Architecture Phase 7.5).
 */

#include "core/CapabilitySet.h"

namespace NetDiscovery {

CapabilitySet& CapabilitySet::operator=(const std::vector<CapabilityModel>& vecCapabilities) {
    m_capabilities.clear();
    for (const auto& cap : vecCapabilities) {
        if (!cap.id.empty()) {
            m_capabilities[cap.id] = cap;
        }
    }
    return *this;
}

bool CapabilitySet::AddCapability(const CapabilityModel& cap) {
    if (cap.id.empty()) return false;
    m_capabilities[cap.id] = cap;
    return true;
}

bool CapabilitySet::RemoveCapability(const std::string& capId) {
    if (capId.empty()) return false;
    return (m_capabilities.erase(capId) > 0);
}

void CapabilitySet::MergeCapabilities(const CapabilitySet& other) {
    if (this == &other) return;
    for (const auto& [id, cap] : other.m_capabilities) {
        m_capabilities[id] = cap;
    }
}

void CapabilitySet::Clear() {
    m_capabilities.clear();
}

bool CapabilitySet::HasCapability(const std::string& capId) const {
    if (capId.empty()) return false;
    return m_capabilities.find(capId) != m_capabilities.end();
}

std::optional<CapabilityModel> CapabilitySet::FindCapability(const std::string& capId) const {
    if (capId.empty()) return std::nullopt;
    auto it = m_capabilities.find(capId);
    if (it != m_capabilities.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<CapabilityModel> CapabilitySet::GetCapabilities() const {
    std::vector<CapabilityModel> result;
    result.reserve(m_capabilities.size());
    for (const auto& [id, cap] : m_capabilities) {
        result.push_back(cap);
    }
    return result;
}

} // namespace NetDiscovery
