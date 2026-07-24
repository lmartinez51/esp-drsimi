/**
 * @file DeviceLifecycleManager.h
 * @brief Device Lifecycle Governance Manager for ESP-Claw Platform (v5.0.0 Architecture).
 */

#pragma once

#include "core/LifecyclePolicy.h"
#include "core/LifecycleTransition.h"
#include "core/KnowledgeEntity.h"
#include "core/StorageEventBus.h"
#include "persistence/IKnowledgeRepository.h"

#include <ctime>
#include <mutex>
#include <string>
#include <memory>

namespace NetDiscovery {

/**
 * @brief Governs entity temporal lifecycle state transitions, retention policies, and counter accounting.
 */
class DeviceLifecycleManager {
public:
    explicit DeviceLifecycleManager(
        Persistence::IKnowledgeRepository& repository,
        LifecyclePolicy policy = LifecyclePolicy{},
        std::shared_ptr<StorageEventBus> eventBus = nullptr);
    
    ~DeviceLifecycleManager() = default;

    /**
     * @brief Record an observation for an entity (updates last_seen, times_seen, restores ACTIVE state).
     */
    void OnObservation(const std::string& entityId);

    /**
     * @brief Record a successful control operation (updates last_success, times_used, restores ACTIVE state).
     */
    void OnSuccessfulControl(const std::string& entityId);

    /**
     * @brief Record a failed control operation (updates times_failed).
     */
    void OnFailedControl(const std::string& entityId);

    /**
     * @brief Evaluate temporal decay and lifecycle transitions for all entities.
     * @param now Current epoch timestamp in seconds.
     */
    void RunMaintenance(std::time_t now);

    /**
     * @brief Update the global or default lifecycle policy.
     */
    void SetPolicy(const LifecyclePolicy& policy);

    /**
     * @brief Get the current lifecycle policy.
     */
    LifecyclePolicy GetPolicy() const;

private:
    void EvaluateEntity(KnowledgeEntity& entity, std::time_t now);
    void TransitionState(KnowledgeEntity& entity, const std::string& newState, const std::string& reason, std::time_t now);
    void PublishEvent(StorageEventType type, const std::string& entityId, const std::string& reason);

    Persistence::IKnowledgeRepository& m_repository;
    LifecyclePolicy m_policy;
    std::shared_ptr<StorageEventBus> m_eventBus;
    mutable std::mutex m_managerMutex;
};

} // namespace NetDiscovery
