/**
 * @file StorageEventBus.cpp
 * @brief Implementation of StorageEventBus.
 */

#include "core/StorageEventBus.h"

namespace NetDiscovery {

StorageEventBus::StorageEventBus() : m_nextSubscriptionId(1) {}

StorageEventBus::SubscriptionId StorageEventBus::Subscribe(StorageEventType type, EventCallback callback) {
    if (!callback) return 0;

    std::lock_guard<std::mutex> lock(m_busMutex);
    SubscriptionId id = m_nextSubscriptionId++;

    SubscriptionRecord record{id, type, callback};
    m_subscriptions[id] = record;

    return id;
}

void StorageEventBus::Unsubscribe(SubscriptionId id) {
    if (id == 0) return;

    std::lock_guard<std::mutex> lock(m_busMutex);
    m_subscriptions.erase(id);
}

void StorageEventBus::Publish(const StorageEvent& event) {
    std::vector<EventCallback> callbacksToInvoke;

    {
        // 1. Copy relevant callbacks while holding lock to avoid deadlocks & iterator invalidation
        std::lock_guard<std::mutex> lock(m_busMutex);
        for (const auto& [id, record] : m_subscriptions) {
            if (record.type == event.type) {
                callbacksToInvoke.push_back(record.callback);
            }
        }
    }

    // 2. Safely invoke callbacks outside m_busMutex lock
    for (const auto& cb : callbacksToInvoke) {
        if (cb) {
            cb(event);
        }
    }
}

size_t StorageEventBus::GetSubscriptionCount() const {
    std::lock_guard<std::mutex> lock(m_busMutex);
    return m_subscriptions.size();
}

} // namespace NetDiscovery
