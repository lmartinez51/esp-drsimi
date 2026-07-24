/**
 * @file DiscoveryManager.h
 * @brief High-level discovery orchestrator managing providers and device registry (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/IDiscoveryProvider.h"
#include "discovery/DeviceRegistry.h"
#include "discovery/DiscoveryEvent.h"

#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace NetDiscovery {
namespace Discovery {

using DiscoveryEventListener = std::function<void(const DiscoveryEvent&)>;

/**
 * @brief Discovery manager managing multiple discovery providers, deduplicating devices, and updating DeviceRegistry.
 *
 * Contains ZERO Runtime dependencies.
 */
class DiscoveryManager {
public:
    explicit DiscoveryManager(std::shared_ptr<DeviceRegistry> registry = nullptr);

    ~DiscoveryManager() = default;

    void RegisterProvider(std::shared_ptr<IDiscoveryProvider> provider);
    void RegisterEventListener(DiscoveryEventListener listener);

    bool StartAllDiscovery();
    void StopAllDiscovery();
    void RefreshAll();

    const DeviceRegistry& GetRegistry() const { return *m_registry; }
    DeviceRegistry& GetRegistry() { return *m_registry; }

private:
    void NotifyListeners(const DiscoveryEvent& event);

    std::shared_ptr<DeviceRegistry> m_registry;
    mutable std::mutex m_mutex;
    std::vector<std::shared_ptr<IDiscoveryProvider>> m_providers;
    std::vector<DiscoveryEventListener> m_listeners;
};

} // namespace Discovery
} // namespace NetDiscovery
