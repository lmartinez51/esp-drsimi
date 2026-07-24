/**
 * @file AdapterLifecycleManager.cpp
 * @brief Implementation of AdapterLifecycleManager (v5.0.0 Architecture Phase 10).
 */

#include "protocol/AdapterLifecycleManager.h"
#include "core/StorageEvent.h"

namespace NetDiscovery {
namespace Protocol {

AdapterLifecycleManager::AdapterLifecycleManager(ProtocolAdapterRegistry* registry,
                                                   StorageEventBus*         eventBus)
    : m_registry(registry)
    , m_eventBus(eventBus) {}

void AdapterLifecycleManager::SetEventBus(StorageEventBus* eventBus) {
    m_eventBus = eventBus;
}

// ── Private State Helper ──────────────────────────────────────────────────────

ProtocolAdapterState& AdapterLifecycleManager::GetOrCreateState(const AdapterId& adapterId) {
    auto it = m_states.find(adapterId);
    if (it == m_states.end()) {
        m_states.emplace(adapterId, ProtocolAdapterState(adapterId));
        it = m_states.find(adapterId);
    }
    return it->second;
}

// ── Private Event Publisher ───────────────────────────────────────────────────

void AdapterLifecycleManager::PublishLifecycleEvent(StorageEventType type,
                                                      const AdapterId& adapterId,
                                                      const std::string& detail) {
    if (!m_eventBus) return;
    StorageEvent event;
    event.type     = type;
    event.entityId = adapterId;
    event.metadata["adapterId"] = adapterId;
    if (!detail.empty()) {
        event.metadata["detail"] = detail;
    }
    m_eventBus->Publish(event);
}

// ── Initialization ────────────────────────────────────────────────────────────

bool AdapterLifecycleManager::InitializeAdapter(const AdapterId& adapterId) {
    if (!m_registry) return false;

    auto adapter = m_registry->Find(adapterId);
    if (!adapter) return false;

    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        auto& state = GetOrCreateState(adapterId);

        if (state.lifecycleState == AdapterLifecycleState::Initialized ||
            state.lifecycleState == AdapterLifecycleState::Available) {
            return true;  // idempotent
        }

        state.lifecycleState = AdapterLifecycleState::Initializing;
        state.BumpVersion();
    }

    const bool ok = adapter->Initialize();

    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        auto& state = GetOrCreateState(adapterId);
        if (ok) {
            state.lifecycleState = AdapterLifecycleState::Initialized;
            state.lastError.clear();
        } else {
            state.lifecycleState = AdapterLifecycleState::Error;
            state.lastError      = "Initialize() returned false";
        }
        state.BumpVersion();
    }

    if (ok) {
        PublishLifecycleEvent(StorageEventType::AdapterInitialized, adapterId);

        // Immediately refresh availability after successful init
        RefreshAvailability(adapterId);
    }

    return ok;
}

uint32_t AdapterLifecycleManager::InitializeAll() {
    if (!m_registry) return 0;

    auto ids = m_registry->GetAllAdapterIds();
    uint32_t count = 0;
    for (const auto& id : ids) {
        if (InitializeAdapter(id)) {
            ++count;
        }
    }
    return count;
}

// ── Shutdown ──────────────────────────────────────────────────────────────────

void AdapterLifecycleManager::ShutdownAdapter(const AdapterId& adapterId) {
    if (!m_registry) return;

    auto adapter = m_registry->Find(adapterId);
    if (!adapter) return;

    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        auto& state = GetOrCreateState(adapterId);
        if (state.lifecycleState == AdapterLifecycleState::Shutdown) return;
        state.lifecycleState = AdapterLifecycleState::ShuttingDown;
        state.BumpVersion();
    }

    adapter->Shutdown();

    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        auto& state = GetOrCreateState(adapterId);
        state.lifecycleState = AdapterLifecycleState::Shutdown;
        state.BumpVersion();
    }

    PublishLifecycleEvent(StorageEventType::AdapterShutdown, adapterId);
}

void AdapterLifecycleManager::ShutdownAll() {
    if (!m_registry) return;
    auto ids = m_registry->GetAllAdapterIds();
    for (const auto& id : ids) {
        ShutdownAdapter(id);
    }
}

// ── Health & Availability ─────────────────────────────────────────────────────

bool AdapterLifecycleManager::RefreshAvailability(const AdapterId& adapterId) {
    if (!m_registry) return false;

    auto adapter = m_registry->Find(adapterId);
    if (!adapter) return false;

    const bool nowAvailable = adapter->IsAvailable();

    AdapterLifecycleState previousState{};
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        auto& state  = GetOrCreateState(adapterId);
        previousState = state.lifecycleState;

        if (nowAvailable) {
            if (state.lifecycleState != AdapterLifecycleState::Busy) {
                state.lifecycleState = AdapterLifecycleState::Available;
            }
        } else {
            if (state.lifecycleState == AdapterLifecycleState::Available ||
                state.lifecycleState == AdapterLifecycleState::Initialized) {
                state.lifecycleState = AdapterLifecycleState::Offline;
            }
        }
        state.BumpVersion();
    }

    // Publish availability change events
    if (nowAvailable && previousState != AdapterLifecycleState::Available) {
        PublishLifecycleEvent(StorageEventType::AdapterAvailable, adapterId);
    } else if (!nowAvailable && previousState == AdapterLifecycleState::Available) {
        PublishLifecycleEvent(StorageEventType::AdapterUnavailable, adapterId);
    }

    PublishLifecycleEvent(StorageEventType::AdapterHealthRefreshed, adapterId);
    return nowAvailable;
}

uint32_t AdapterLifecycleManager::RefreshAllAvailability() {
    if (!m_registry) return 0;
    auto ids = m_registry->GetAllAdapterIds();
    uint32_t available = 0;
    for (const auto& id : ids) {
        if (RefreshAvailability(id)) {
            ++available;
        }
    }
    return available;
}

// ── State Observation ─────────────────────────────────────────────────────────

std::optional<ProtocolAdapterState>
AdapterLifecycleManager::GetAdapterState(const AdapterId& adapterId) const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    auto it = m_states.find(adapterId);
    if (it == m_states.end()) return std::nullopt;
    return it->second;
}

std::vector<ProtocolAdapterState>
AdapterLifecycleManager::GetAllAdapterStates() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    std::vector<ProtocolAdapterState> result;
    result.reserve(m_states.size());
    for (const auto& kv : m_states) {
        result.push_back(kv.second);
    }
    return result;
}

std::vector<AdapterId>
AdapterLifecycleManager::GetAvailableAdapterIds() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    std::vector<AdapterId> ids;
    for (const auto& kv : m_states) {
        if (kv.second.lifecycleState == AdapterLifecycleState::Available) {
            ids.push_back(kv.first);
        }
    }
    return ids;
}

} // namespace Protocol
} // namespace NetDiscovery
