/**
 * @file DeviceIdentityContext.h
 * @brief Mutable runtime context for logical device identities (v5.0.0 Architecture Phase 18).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Identity {

/**
 * @brief Mutable runtime context owned exclusively by IdentityManager.
 */
struct DeviceIdentityContext {
    std::vector<std::string> aliases;
    std::string displayName;
    std::string assignedRoom;
    bool        preferred{false};
    bool        enabled{true};
    std::unordered_map<std::string, std::string> userMetadata;
    std::vector<std::string> linkedDiscoveryIds;
    uint64_t    lastSeenMs{0};
    std::vector<std::string> customTags;

    DeviceIdentityContext() = default;

    explicit DeviceIdentityContext(std::string name)
        : displayName(std::move(name)) {}

    bool HasAlias(const std::string& alias) const {
        for (const auto& a : aliases) {
            if (a == alias) return true;
        }
        return false;
    }

    bool IsLinkedTo(const std::string& discoveryId) const {
        for (const auto& id : linkedDiscoveryIds) {
            if (id == discoveryId) return true;
        }
        return false;
    }
};

} // namespace Identity
} // namespace NetDiscovery
