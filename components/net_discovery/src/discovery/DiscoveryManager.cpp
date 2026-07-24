/**
 * @file DiscoveryManager.cpp
 * @brief Implementation of DiscoveryManager (v5.0.0 Architecture Phase 17).
 */

#include "discovery/DiscoveryManager.h"

namespace NetDiscovery {
namespace Discovery {

DiscoveryManager::DiscoveryManager(std::shared_ptr<DeviceRegistry> registry)
    : m_registry(std::move(registry)) {

    if (!m_registry) {
        m_registry = std::make_shared<DeviceRegistry>();
    }
}

void DiscoveryManager::RegisterProvider(std::shared_ptr<IDiscoveryProvider> provider) {
    if (!provider) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_providers.push_back(std::move(provider));
}

void DiscoveryManager::RegisterEventListener(DiscoveryEventListener listener) {
    if (!listener) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.push_back(std::move(listener));
}

bool DiscoveryManager::StartAllDiscovery() {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool success = true;
    for (auto& provider : m_providers) {
        if (provider) {
            if (!provider->StartDiscovery()) {
                success = false;
            } else {
                auto devices = provider->GetDiscoveredDevices();
                for (auto& dev : devices) {
                    m_registry->RegisterDevice(dev);
                    NotifyListeners(DiscoveryEvent(DiscoveryEventType::DeviceDiscovered, provider->GetProviderId(), dev));
                }
            }
        }
    }
    return success;
}

void DiscoveryManager::StopAllDiscovery() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& provider : m_providers) {
        if (provider) {
            provider->StopDiscovery();
        }
    }
}

void DiscoveryManager::RefreshAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& provider : m_providers) {
        if (provider) {
            provider->Refresh();
            auto devices = provider->GetDiscoveredDevices();
            for (auto& dev : devices) {
                m_registry->RegisterDevice(dev);
                NotifyListeners(DiscoveryEvent(DiscoveryEventType::DeviceUpdated, provider->GetProviderId(), dev));
            }
        }
    }
}

void DiscoveryManager::NotifyListeners(const DiscoveryEvent& event) {
    for (const auto& listener : m_listeners) {
        if (listener) listener(event);
    }
}

} // namespace Discovery
} // namespace NetDiscovery
