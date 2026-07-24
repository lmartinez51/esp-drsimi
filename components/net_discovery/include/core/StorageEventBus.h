/**
 * @file StorageEventBus.h
 * @brief Thread-safe asynchronous Domain Event Bus for ESP-Claw Platform (Architecture v5.0.0).
 */

#pragma once

#include "core/StorageEvent.h"
#include "core/StorageEventSubscriber.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>

namespace NetDiscovery {

/**
 * @brief Decoupled, thread-safe Event Bus facilitating platform-wide domain event distribution.
 */
class StorageEventBus {
public:
    using SubscriptionId = uint64_t;
    using EventCallback = std::function<void(const StorageEvent&)>;

    StorageEventBus();
    ~StorageEventBus() = default;

    // Non-copyable, non-movable
    StorageEventBus(const StorageEventBus&) = delete;
    StorageEventBus& operator=(const StorageEventBus&) = delete;

    /**
     * @brief Subscribe a callback to a specific StorageEventType topic.
     * @return Unique SubscriptionId for unsubscribing later.
     */
    SubscriptionId Subscribe(StorageEventType type, EventCallback callback);

    /**
     * @brief Unsubscribe a subscriber using its SubscriptionId.
     */
    void Unsubscribe(SubscriptionId id);

    /**
     * @brief Publish a StorageEvent to all topic subscribers safely without deadlocks.
     */
    void Publish(const StorageEvent& event);

    /**
     * @brief Get total active subscription count.
     */
    size_t GetSubscriptionCount() const;

private:
    struct SubscriptionRecord {
        SubscriptionId id;
        StorageEventType type;
        EventCallback callback;
    };

    SubscriptionId m_nextSubscriptionId{1};
    std::unordered_map<SubscriptionId, SubscriptionRecord> m_subscriptions;
    mutable std::mutex m_busMutex;
};

} // namespace NetDiscovery
