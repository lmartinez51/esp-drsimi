/**
 * @file ProtocolCapabilitySet.h
 * @brief Immutable collection of ProtocolCapability instances (v5.0.0 Architecture Phase 13).
 */

#pragma once

#include "protocol/capability/ProtocolCapability.h"

#include <vector>
#include <optional>
#include <algorithm>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Container holding a set of ProtocolCapability instances supported by a protocol adapter.
 *
 * Provides query methods Contains(), Supports(), Find(), and Enumerate().
 */
class ProtocolCapabilitySet {
public:
    ProtocolCapabilitySet() = default;

    explicit ProtocolCapabilitySet(std::vector<ProtocolCapability> capabilities)
        : m_capabilities(std::move(capabilities)) {}

    void AddCapability(ProtocolCapability capability) {
        if (!Contains(capability.capabilityId)) {
            m_capabilities.push_back(std::move(capability));
        }
    }

    bool Contains(const CapabilityId& id) const {
        return std::any_of(m_capabilities.begin(), m_capabilities.end(),
            [&id](const ProtocolCapability& cap) { return cap.capabilityId == id; });
    }

    bool Supports(const ProtocolCapability& cap) const {
        return Contains(cap.capabilityId);
    }

    std::optional<ProtocolCapability> Find(const CapabilityId& id) const {
        for (const auto& cap : m_capabilities) {
            if (cap.capabilityId == id) return cap;
        }
        return std::nullopt;
    }

    const std::vector<ProtocolCapability>& Enumerate() const {
        return m_capabilities;
    }

    bool IsEmpty() const { return m_capabilities.empty(); }
    std::size_t Size() const { return m_capabilities.size(); }

private:
    std::vector<ProtocolCapability> m_capabilities;
};

} // namespace Protocol
} // namespace NetDiscovery
