/**
 * @file StorageEventSubscriber.h
 * @brief Subscriber interface for receiving domain events from StorageEventBus.
 */

#pragma once

#include "core/StorageEvent.h"

namespace NetDiscovery {

/**
 * @brief Pure virtual subscriber interface for event handlers.
 */
class IStorageEventSubscriber {
public:
    virtual ~IStorageEventSubscriber() = default;

    /**
     * @brief Callback executed when a subscribed StorageEvent is published.
     */
    virtual void OnStorageEvent(const StorageEvent& event) = 0;
};

} // namespace NetDiscovery
