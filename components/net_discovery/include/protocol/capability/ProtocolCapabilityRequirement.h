/**
 * @file ProtocolCapabilityRequirement.h
 * @brief Immutable descriptor representing protocol capability requirements for an ExecutionStep (v5.0.0 Architecture Phase 13).
 */

#pragma once

#include "protocol/capability/ProtocolCapability.h"

#include <vector>
#include <unordered_map>
#include <utility>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Immutable descriptor carrying capability requirements for an ExecutionStep.
 */
class ProtocolCapabilityRequirement {
public:
    ProtocolCapabilityRequirement() = default;

    explicit ProtocolCapabilityRequirement(std::vector<CapabilityId> required,
                                           std::vector<CapabilityId> optional = {},
                                           std::unordered_map<std::string, std::string> metadata = {})
        : m_requiredCapabilityIds(std::move(required))
        , m_optionalCapabilityIds(std::move(optional))
        , m_metadata(std::move(metadata)) {}

    const std::vector<CapabilityId>& GetRequiredCapabilities() const { return m_requiredCapabilityIds; }
    const std::vector<CapabilityId>& GetOptionalCapabilities() const { return m_optionalCapabilityIds; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const { return m_metadata; }

    bool IsEmpty() const { return m_requiredCapabilityIds.empty() && m_optionalCapabilityIds.empty(); }

private:
    std::vector<CapabilityId> m_requiredCapabilityIds;
    std::vector<CapabilityId> m_optionalCapabilityIds;
    std::unordered_map<std::string, std::string> m_metadata;
};

} // namespace Protocol
} // namespace NetDiscovery
