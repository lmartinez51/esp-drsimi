/**
 * @file FileKnowledgeRepository.h
 * @brief File-backed implementation of IKnowledgeRepository using AtomicFileWriter and IKnowledgeSerializer.
 * ESP-Claw Platform Knowledge Memory Owner.
 */

#pragma once

#include "persistence/IKnowledgeRepository.h"
#include "persistence/RepositoryContext.h"
#include "persistence/IKnowledgeSerializer.h"

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

namespace NetDiscovery {
namespace Persistence {

/**
 * @brief File-backed implementation of IKnowledgeRepository.
 * Manages ESP-Claw persistent knowledge entity records.
 */
class FileKnowledgeRepository : public IKnowledgeRepository {
public:
    explicit FileKnowledgeRepository(
        std::shared_ptr<RepositoryContext> context,
        std::unique_ptr<IKnowledgeSerializer> serializer = nullptr);
    ~FileKnowledgeRepository() override = default;

    // Entity Mutation & Lookup
    bool SaveEntity(const KnowledgeEntity& entity) override;
    std::optional<KnowledgeEntity> FindById(const std::string& entityId) override;
    std::optional<KnowledgeEntity> FindByMac(const std::string& macAddress) override;
    std::vector<KnowledgeEntity> GetAllEntities() override;
    std::vector<KnowledgeEntity> FindByCapability(const std::string& capability) override;
    std::vector<KnowledgeEntity> FindByVendor(const std::string& vendor) override;
    std::vector<KnowledgeEntity> FindByRoom(const std::string& roomEntityId) override;

    // Semantic & Reachability Queries
    std::vector<KnowledgeEntity> FindActiveDevices() override;
    std::vector<KnowledgeEntity> FindReachableDevices() override;
    std::vector<KnowledgeEntity> FindControllerCandidates(const std::string& capabilityName) override;
    std::vector<KnowledgeEntity> FindEntitiesUsingProtocol(const std::string& protocol) override;

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
    void ReloadCacheLocked();

    std::shared_ptr<RepositoryContext> m_context;
    std::unique_ptr<IKnowledgeSerializer> m_serializer;
    std::string m_storageDir;
    mutable std::mutex m_mutex;

    std::map<std::string, KnowledgeEntity> m_cache;
    bool m_cacheLoaded{false};
};

} // namespace Persistence
} // namespace NetDiscovery
