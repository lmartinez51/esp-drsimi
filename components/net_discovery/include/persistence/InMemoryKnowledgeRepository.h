/**
 * @file InMemoryKnowledgeRepository.h
 * @brief High-Performance In-Memory Knowledge Repository with Multi-Indexing and Batched Atomic Storage Flushes.
 * Primary in-memory data store for ESP-Claw Platform Knowledge.
 */

#pragma once

#include "persistence/IKnowledgeRepository.h"
#include "persistence/RepositoryContext.h"
#include "persistence/IKnowledgeSerializer.h"
#include "core/StorageEventBus.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>

namespace NetDiscovery {
namespace Persistence {

class InMemoryKnowledgeRepository : public IKnowledgeRepository {
public:
    explicit InMemoryKnowledgeRepository(
        std::shared_ptr<RepositoryContext> context,
        std::unique_ptr<IKnowledgeSerializer> serializer = nullptr,
        std::shared_ptr<StorageEventBus> eventBus = nullptr);

    ~InMemoryKnowledgeRepository() override;

    // Startup Initializer & Storage Sync
    bool LoadFromStorage() override;
    bool FlushDirty() override;
    bool FlushAll() override;
    RepositoryStats GetStats() override;

    // Entity Mutation & Lookup (O(1) / Fast In-Memory Operations)
    bool SaveEntity(const KnowledgeEntity& entity) override;
    std::optional<KnowledgeEntity> FindById(const std::string& entityId) override;
    std::optional<KnowledgeEntity> FindByMac(const std::string& macAddress) override;
    std::vector<KnowledgeEntity> FindByCapability(const std::string& capability) override;
    std::vector<KnowledgeEntity> FindByVendor(const std::string& vendor) override;
    std::vector<KnowledgeEntity> FindByRoom(const std::string& roomEntityId) override;

    // Semantic & Reachability Queries
    std::vector<KnowledgeEntity> FindActiveDevices() override;
    std::vector<KnowledgeEntity> FindReachableDevices() override;
    std::vector<KnowledgeEntity> FindControllerCandidates(const std::string& controllerName) override;
    std::vector<KnowledgeEntity> FindEntitiesUsingProtocol(const std::string& protocol) override;
    std::vector<KnowledgeEntity> GetAllEntities() override;

    // Graph & Relationship Queries
    bool LinkEntities(const std::string& sourceId, const std::string& targetId, RelationshipType type) override;
    std::vector<KnowledgeEntity> GetRelatedEntities(const std::string& entityId, RelationshipType type) override;

    // Lifecycle Management
    bool SoftDeleteEntity(const std::string& entityId) override;
    bool RestoreEntity(const std::string& entityId) override;
    bool PermanentPurge(const std::string& entityId) override;

private:
    void EnsureDirectoryExists();
    std::string GetEntityFilePath(const std::string& entityId) const;

    // Internal Index Maintenance (Must be called under m_mutex)
    void BuildIndexesForEntityLocked(const KnowledgeEntity& entity);
    void RemoveIndexesForEntityLocked(const KnowledgeEntity& entity);
    void RebuildAllIndexesLocked();
    void PublishEvent(StorageEventType type, const std::string& entityId, const std::unordered_map<std::string, std::string>& meta = {});

    std::shared_ptr<RepositoryContext> m_context;
    std::unique_ptr<IKnowledgeSerializer> m_serializer;
    std::shared_ptr<StorageEventBus> m_eventBus;
    std::string m_storageDir;
    mutable std::mutex m_mutex;

    // Live In-Memory Master Entity Store
    std::unordered_map<std::string, KnowledgeEntity> m_entities;

    // Multi-Index Structures for Fast Search
    std::unordered_map<std::string, std::string> m_macIndex;                           // MAC -> EntityId
    std::unordered_map<std::string, std::vector<std::string>> m_vendorIndex;           // Vendor -> [EntityId]
    std::unordered_map<std::string, std::vector<std::string>> m_capabilityIndex;       // Capability -> [EntityId]
    std::unordered_map<std::string, std::vector<std::string>> m_controllerIndex;       // ControllerName -> [EntityId]
    std::unordered_map<std::string, std::vector<std::string>> m_roomIndex;             // RoomTargetId -> [EntityId]

    // Dirty Entity Tracking
    std::unordered_set<std::string> m_dirtyEntityIds;
    int64_t m_lastFlushTimestamp{0};
    bool m_isLoaded{false};
};

} // namespace Persistence
} // namespace NetDiscovery
