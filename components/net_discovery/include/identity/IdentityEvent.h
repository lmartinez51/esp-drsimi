/**
 * @file IdentityEvent.h
 * @brief Immutable events for identity lifecycle state transitions (v5.0.0 Architecture Phase 18).
 */

#pragma once

#include "identity/DeviceIdentity.h"
#include <string>
#include <utility>

namespace NetDiscovery {
namespace Identity {

enum class IdentityEventType {
    IdentityCreated,
    IdentityUpdated,
    IdentityLinked,
    IdentityRenamed,
    IdentityDisabled,
    IdentityRemoved
};

inline std::string ToString(IdentityEventType type) {
    switch (type) {
        case IdentityEventType::IdentityCreated:  return "IdentityCreated";
        case IdentityEventType::IdentityUpdated:  return "IdentityUpdated";
        case IdentityEventType::IdentityLinked:   return "IdentityLinked";
        case IdentityEventType::IdentityRenamed:  return "IdentityRenamed";
        case IdentityEventType::IdentityDisabled: return "IdentityDisabled";
        case IdentityEventType::IdentityRemoved:  return "IdentityRemoved";
        default:                                  return "Unknown";
    }
}

struct IdentityEvent {
    IdentityEventType type{IdentityEventType::IdentityCreated};
    IdentityId        identityId;
    std::string       details;
    uint64_t          timestampMs{0};

    IdentityEvent() = default;

    IdentityEvent(IdentityEventType t, IdentityId id, std::string d = "", uint64_t ts = 0)
        : type(t), identityId(std::move(id)), details(std::move(d)), timestampMs(ts) {}
};

} // namespace Identity
} // namespace NetDiscovery
