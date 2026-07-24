/**
 * @file StorageEvent.h
 * @brief Generic domain event payload structure for StorageEventBus.
 */

#pragma once

#include "core/StorageEventType.h"

#include <string>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {

/**
 * @brief Domain event object payload.
 */
struct StorageEvent {
    StorageEventType type{StorageEventType::EntityUpdated};
    std::string entityId;
    uint64_t timestamp{0};
    std::unordered_map<std::string, std::string> metadata;
};

} // namespace NetDiscovery
