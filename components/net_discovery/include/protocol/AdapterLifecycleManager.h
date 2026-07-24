/**
 * @file AdapterLifecycleManager.h
 * @brief Manages adapter initialization, shutdown, and health state without executing protocol operations (v5.0.0 Architecture Phase 10).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"
#include "protocol/ProtocolAdapterRegistry.h"
#include "protocol/ProtocolAdapterState.h"
#include "core/StorageEventBus.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Orchestrates adapter lifecycle transitions and health state tracking.
 *
 * AdapterLifecycleManager is the only component allowed to call Initialize()
 * and Shutdown() on IProtocolAdapter instances. RuntimeExecutionEngine is not
 * permitted to call lifecycle methods directly.
 *
 * Responsibilities:
 *  - Initialize adapters (transition Uninitialized → Initialized → Available).
 *  - Shutdown adapters (transition any state → ShuttingDown → Shutdown).
 *  - Refresh availability (call IsAvailable() and update ProtocolAdapterState).
 *  - Publish lifecycle events to StorageEventBus (metadata only, no protocol data).
 *
 * This class never calls IProtocolAdapter::Execute(). Execution is exclusively
 * the responsibility of RuntimeExecutionEngine via IExecutionDispatcher.
 */
class AdapterLifecycleManager {
public:
    /**
     * @param registry  Registry from which adapters are sourced.
     * @param eventBus  Optional event bus for lifecycle notifications.
     */
    explicit AdapterLifecycleManager(ProtocolAdapterRegistry* registry,
                                     StorageEventBus*         eventBus = nullptr);

    ~AdapterLifecycleManager() = default;

    // Non-copyable
    AdapterLifecycleManager(const AdapterLifecycleManager&) = delete;
    AdapterLifecycleManager& operator=(const AdapterLifecycleManager&) = delete;

    void SetEventBus(StorageEventBus* eventBus);

    // ── Initialization ──────────────────────────────────────────────────────

    /**
     * @brief Calls Initialize() on the adapter with the given ID.
     *
     * Transitions state: Uninitialized → Initialized → Available (if IsAvailable() returns true).
     * Publishes AdapterInitialized and optionally AdapterAvailable.
     *
     * @return true if initialization succeeded and adapter is now initialized.
     */
    bool InitializeAdapter(const AdapterId& adapterId);

    /**
     * @brief Calls Initialize() on every adapter currently registered.
     *
     * @return Number of adapters successfully initialized.
     */
    uint32_t InitializeAll();

    // ── Shutdown ────────────────────────────────────────────────────────────

    /**
     * @brief Calls Shutdown() on the adapter with the given ID.
     *
     * Transitions state: any → ShuttingDown → Shutdown.
     * Publishes AdapterShutdown.
     */
    void ShutdownAdapter(const AdapterId& adapterId);

    /**
     * @brief Calls Shutdown() on every registered adapter.
     */
    void ShutdownAll();

    // ── Health & Availability ───────────────────────────────────────────────

    /**
     * @brief Calls IsAvailable() on the given adapter and updates its ProtocolAdapterState.
     *
     * Publishes AdapterAvailable or AdapterUnavailable if the state changed.
     * Publishes AdapterHealthRefreshed unconditionally.
     *
     * @return Current availability.
     */
    bool RefreshAvailability(const AdapterId& adapterId);

    /**
     * @brief Refreshes availability for all registered adapters.
     *
     * @return Number of adapters currently available after refresh.
     */
    uint32_t RefreshAllAvailability();

    // ── State Observation ───────────────────────────────────────────────────

    /**
     * @brief Returns a snapshot of the current state for the given adapter.
     */
    std::optional<ProtocolAdapterState> GetAdapterState(const AdapterId& adapterId) const;

    /**
     * @brief Returns snapshots of state for all managed adapters.
     */
    std::vector<ProtocolAdapterState> GetAllAdapterStates() const;

    /**
     * @brief Returns the IDs of all adapters currently in Available state.
     */
    std::vector<AdapterId> GetAvailableAdapterIds() const;

private:
    void PublishLifecycleEvent(StorageEventType type, const AdapterId& adapterId,
                               const std::string& detail = "");

    ProtocolAdapterState& GetOrCreateState(const AdapterId& adapterId);

    ProtocolAdapterRegistry* m_registry{nullptr};
    StorageEventBus*         m_eventBus{nullptr};

    mutable std::mutex m_stateMutex;
    std::unordered_map<AdapterId, ProtocolAdapterState> m_states;
};

} // namespace Protocol
} // namespace NetDiscovery
