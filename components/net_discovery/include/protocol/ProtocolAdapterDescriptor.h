/**
 * @file ProtocolAdapterDescriptor.h
 * @brief Immutable metadata describing a protocol adapter (v5.0.0 Architecture Phase 10).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Strongly typed protocol adapter identifier.
 */
using AdapterId = std::string;

/**
 * @brief Immutable value object carrying static metadata about a protocol adapter.
 *
 * Created once at registration time. Never mutated after construction.
 * Contains no mutable state, no runtime variables, no health information.
 * Runtime health and availability are carried by ProtocolAdapterState.
 */
struct ProtocolAdapterDescriptor {
    AdapterId   adapterId;              ///< Unique identifier within the registry.
    std::string protocolName;           ///< Human-readable protocol name (e.g., "UPnP", "BLE", "Matter").
    std::string version;                ///< Adapter implementation version string.
    std::string vendor;                 ///< Adapter vendor or origin identifier.
    std::string transport;              ///< Underlying transport mechanism (e.g., "TCP", "BLE", "IR").

    /// Operations this adapter can execute (mapped to OperationDefinition IDs).
    std::vector<std::string> supportedOperations;

    /// Capability tags this adapter declares (e.g., "power", "volume", "media").
    std::vector<std::string> supportedCapabilities;

    /// Arbitrary extensible metadata for protocol-specific properties.
    std::unordered_map<std::string, std::string> metadata;

    ProtocolAdapterDescriptor() = default;

    ProtocolAdapterDescriptor(AdapterId id, std::string protocol, std::string ver,
                               std::string ven, std::string trans,
                               std::vector<std::string> ops = {},
                               std::vector<std::string> caps = {},
                               std::unordered_map<std::string, std::string> meta = {})
        : adapterId(std::move(id))
        , protocolName(std::move(protocol))
        , version(std::move(ver))
        , vendor(std::move(ven))
        , transport(std::move(trans))
        , supportedOperations(std::move(ops))
        , supportedCapabilities(std::move(caps))
        , metadata(std::move(meta)) {}

    bool IsValid() const { return !adapterId.empty() && !protocolName.empty(); }

    bool SupportsOperation(const std::string& operationId) const {
        for (const auto& op : supportedOperations) {
            if (op == operationId) return true;
        }
        return false;
    }

    bool SupportsCapability(const std::string& capability) const {
        for (const auto& cap : supportedCapabilities) {
            if (cap == capability) return true;
        }
        return false;
    }
};

} // namespace Protocol
} // namespace NetDiscovery
