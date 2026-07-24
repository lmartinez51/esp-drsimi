/**
 * @file DeviceLifecycleManager.cpp
 * @brief Implementation of Device Lifecycle Manager with StorageEventBus integration.
 */

#include "services/DeviceLifecycleManager.h"

#include <chrono>
#include "esp_log.h"

static const char* TAG = "DeviceLifecycleManager";

namespace NetDiscovery {

DeviceLifecycleManager::DeviceLifecycleManager(
    Persistence::IKnowledgeRepository& repository,
    LifecyclePolicy policy,
    std::shared_ptr<StorageEventBus> eventBus)
    : m_repository(repository),
      m_policy(policy),
      m_eventBus(eventBus)
{
}

void DeviceLifecycleManager::SetPolicy(const LifecyclePolicy& policy) {
    std::lock_guard<std::mutex> lock(m_managerMutex);
    m_policy = policy;
}

LifecyclePolicy DeviceLifecycleManager::GetPolicy() const {
    std::lock_guard<std::mutex> lock(m_managerMutex);
    return m_policy;
}

void DeviceLifecycleManager::OnObservation(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_managerMutex);

    auto entityOpt = m_repository.FindById(entityId);
    if (!entityOpt.has_value()) {
        ESP_LOGW(TAG, "OnObservation called for non-existent entity %s", entityId.c_str());
        return;
    }

    KnowledgeEntity entity = entityOpt.value();
    using namespace std::chrono;
    int64_t now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    // 1. Exclusive Counter Updates
    entity.lifecycle.lastSeen = now;
    entity.lifecycle.timesSeen++;
    entity.lifecycle.lastModified = now;
    entity.lastSeen = now; // Backwards compatibility field

    // 2. Runtime State Update
    entity.runtimeState.isOnline = true;

    // 3. State Transition & Recovery
    if (entity.lifecycle.state == "OFFLINE" ||
        entity.lifecycle.state == "STALE" ||
        entity.lifecycle.state == "ARCHIVED") {
        TransitionState(entity, "ACTIVE", "NEW_OBSERVATION", now);
    } else {
        m_repository.SaveEntity(entity);
    }
}

void DeviceLifecycleManager::OnSuccessfulControl(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_managerMutex);

    auto entityOpt = m_repository.FindById(entityId);
    if (!entityOpt.has_value()) {
        ESP_LOGW(TAG, "OnSuccessfulControl called for non-existent entity %s", entityId.c_str());
        return;
    }

    KnowledgeEntity entity = entityOpt.value();
    using namespace std::chrono;
    int64_t now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    // 1. Counter Updates
    entity.lifecycle.lastSuccess = now;
    entity.lifecycle.timesUsed++;
    entity.lifecycle.lastModified = now;

    // 2. Runtime State Update
    entity.runtimeState.isOnline = true;

    // 3. State Transition & Recovery
    if (entity.lifecycle.state == "OFFLINE" ||
        entity.lifecycle.state == "STALE" ||
        entity.lifecycle.state == "ARCHIVED") {
        TransitionState(entity, "ACTIVE", "SUCCESSFUL_CONTROL", now);
    } else {
        m_repository.SaveEntity(entity);
    }
}

void DeviceLifecycleManager::OnFailedControl(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_managerMutex);

    auto entityOpt = m_repository.FindById(entityId);
    if (!entityOpt.has_value()) {
        ESP_LOGW(TAG, "OnFailedControl called for non-existent entity %s", entityId.c_str());
        return;
    }

    KnowledgeEntity entity = entityOpt.value();
    using namespace std::chrono;
    int64_t now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    entity.lifecycle.timesFailed++;
    entity.lifecycle.lastModified = now;

    m_repository.SaveEntity(entity);
}

void DeviceLifecycleManager::RunMaintenance(std::time_t now) {
    std::lock_guard<std::mutex> lock(m_managerMutex);

    auto entities = m_repository.GetAllEntities();
    for (auto entity : entities) {
        EvaluateEntity(entity, now);
    }
}

void DeviceLifecycleManager::EvaluateEntity(KnowledgeEntity& entity, std::time_t now) {
    int64_t currentSec = static_cast<int64_t>(now);
    int64_t elapsedSinceSeen = (entity.lifecycle.lastSeen > 0) ? (currentSec - entity.lifecycle.lastSeen) : 0;
    int64_t elapsedSinceArchived = (entity.lifecycle.archivedAt > 0) ? (currentSec - entity.lifecycle.archivedAt) : 0;

    // Determine thresholds based on retention policy
    int64_t offlineSec = std::chrono::duration_cast<std::chrono::seconds>(m_policy.offlineAfter).count();
    int64_t staleSec   = std::chrono::duration_cast<std::chrono::seconds>(m_policy.staleAfter).count();
    int64_t archiveSec = std::chrono::duration_cast<std::chrono::seconds>(m_policy.archiveAfter).count();
    int64_t purgeSec   = std::chrono::duration_cast<std::chrono::seconds>(m_policy.purgeAfter).count();

    if (entity.lifecycle.retentionPolicy == "TEMPORARY") {
        offlineSec /= 4;
        staleSec /= 4;
        archiveSec /= 4;
        purgeSec /= 4;
    }

    // 1. PINNED policy protection (never archive or purge)
    bool isPinned = (entity.lifecycle.retentionPolicy == "PINNED");

    // State Evaluation Pipeline
    if (entity.lifecycle.state == "ACTIVE") {
        if (elapsedSinceSeen >= offlineSec) {
            entity.runtimeState.isOnline = false;
            TransitionState(entity, "OFFLINE", "INACTIVITY_TIMEOUT", currentSec);
        }
    } else if (entity.lifecycle.state == "OFFLINE") {
        if (elapsedSinceSeen >= staleSec) {
            TransitionState(entity, "STALE", "STALE_TIMEOUT", currentSec);
        }
    } else if (entity.lifecycle.state == "STALE") {
        if (!isPinned && elapsedSinceSeen >= archiveSec) {
            entity.lifecycle.archivedAt = currentSec;
            TransitionState(entity, "ARCHIVED", "ARCHIVE_TIMEOUT", currentSec);
        }
    } else if (entity.lifecycle.state == "ARCHIVED") {
        if (!isPinned && elapsedSinceArchived >= purgeSec) {
            entity.lifecycle.userDeleted = true;
            TransitionState(entity, "SOFT_DELETED", "PURGE_TIMEOUT", currentSec);
        }
    }
}

void DeviceLifecycleManager::TransitionState(
    KnowledgeEntity& entity,
    const std::string& newState,
    const std::string& reason,
    std::time_t now)
{
    std::string prevState = entity.lifecycle.state;
    entity.lifecycle.state = newState;
    entity.lifecycle.lastModified = static_cast<int64_t>(now);
    entity.lifecycle.revision++;

    ESP_LOGI(TAG, "Entity %s state transition: %s -> %s (Reason: %s)",
             entity.persistentId.c_str(), prevState.c_str(), newState.c_str(), reason.c_str());

    m_repository.SaveEntity(entity);

    if (newState == "ARCHIVED") {
        PublishEvent(StorageEventType::EntityArchived, entity.persistentId, reason);
    } else if (newState == "SOFT_DELETED") {
        PublishEvent(StorageEventType::EntityDeleted, entity.persistentId, reason);
    }
}

void DeviceLifecycleManager::PublishEvent(StorageEventType type, const std::string& entityId, const std::string& reason) {
    if (!m_eventBus) return;

    using namespace std::chrono;
    uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    StorageEvent event;
    event.type = type;
    event.entityId = entityId;
    event.timestamp = now;
    event.metadata["reason"] = reason;

    m_eventBus->Publish(event);
}

} // namespace NetDiscovery
