/**
 * @file ProtocolCapability.h
 * @brief Immutable value object representing an intrinsic protocol feature capability (v5.0.0 Architecture Phase 13.1).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

namespace NetDiscovery {
namespace Protocol {

using CapabilityId = std::string;

/**
 * @brief Immutable value object representing an intrinsic feature capability of a protocol adapter.
 *
 * Distinct from device capabilities, semantic capabilities, and DispatcherCapabilities.
 * Represents features like QoS1, SOAP, CASE, Notify, TLS, etc.
 */
struct ProtocolCapability {
    CapabilityId capabilityId;              ///< e.g., "mqtt.qos1", "upnp.soap"
    std::string  name;                       ///< Human-readable capability name
    std::string  description;                ///< Detailed capability description
    std::string  version{"1.0.0"};           ///< Feature version
    std::vector<std::string> tags;           ///< Optional classification tags
    std::string  stability{"Stable"};        ///< "Stable", "Experimental", "Deprecated"
    std::vector<CapabilityId> prerequisites; ///< Hierarchical capability prerequisites
    std::vector<CapabilityId> dependencies;  ///< Dependent capability IDs
    std::vector<std::string>  constraints;   ///< Usage constraints
    std::unordered_map<std::string, std::string> metadata;

    ProtocolCapability() = default;

    ProtocolCapability(CapabilityId id,
                       std::string capName,
                       std::string capDesc = "",
                       std::string ver = "1.0.0",
                       std::vector<std::string> capTags = {},
                       std::string capStability = "Stable",
                       std::vector<CapabilityId> prereqs = {},
                       std::vector<CapabilityId> deps = {},
                       std::vector<std::string> constrs = {},
                       std::unordered_map<std::string, std::string> meta = {})
        : capabilityId(std::move(id))
        , name(std::move(capName))
        , description(std::move(capDesc))
        , version(std::move(ver))
        , tags(std::move(capTags))
        , stability(std::move(capStability))
        , prerequisites(std::move(prereqs))
        , dependencies(std::move(deps))
        , constraints(std::move(constrs))
        , metadata(std::move(meta)) {}

    bool IsValid() const { return !capabilityId.empty() && !name.empty(); }

    bool operator==(const ProtocolCapability& other) const {
        return capabilityId == other.capabilityId;
    }
};

} // namespace Protocol
} // namespace NetDiscovery
