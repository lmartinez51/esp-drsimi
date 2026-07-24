/**
 * @file IKnowledgeRepository.h
 * @brief Semantic Repository Interface for ESP-Claw Knowledge Base.
 */

#pragma once

#include "core/KnowledgeEntity.h"

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>

namespace NetDiscovery {
namespace Persistence {

/**
 * @brief Relationship type for entity graph edges.
 */
enum class RelationshipType {
    LocatedIn,
    Uses,
    PairedWith,
    ControlledBy,
    ParentOf,
    ChildOf,
    GroupMember
};

/**
 * @brief Converts RelationshipType to string.
 */
inline std::string ToString(RelationshipType type) {
    switch (type) {
        case RelationshipType::LocatedIn: return "LOCATED_IN";
        case RelationshipType::Uses: return "USES";
        case RelationshipType::PairedWith: return "PAIRED_WITH";
        case RelationshipType::ControlledBy: return "CONTROLLED_BY";
        case RelationshipType::ParentOf: return "PARENT_OF";
        case RelationshipType::ChildOf: return "CHILD_OF";
        case RelationshipType::GroupMember: return "GROUP_MEMBER";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Statistics structure for in-memory repository diagnostics.
 */
struct RepositoryStats {
    size_t totalEntities{0};
    size_t activeEntities{0};
    size_t softDeletedEntities{0};
    size_t dirtyCount{0};
    size_t indexedMacs{0};
    size_t indexedVendors{0};
    size_t indexedCapabilities{0};
    size_t indexedControllers{0};
    size_t indexedRooms{0};
    int64_t lastFlushTimestamp{0};
};

/**
 * @brief High-level semantic query repository interface for Knowledge Entities.
 */
class IKnowledgeRepository {
public:
    virtual ~IKnowledgeRepository() = default;

    // CRUD & Node Persistence
    virtual std::optional<KnowledgeEntity> FindById(const std::string& entityId) = 0;
    virtual std::optional<KnowledgeEntity> FindByMac(const std::string& macAddress) = 0;
    virtual bool SaveEntity(const KnowledgeEntity& entity) = 0;

    // High-Level Semantic Queries
    virtual std::vector<KnowledgeEntity> FindByCapability(const std::string& capability) = 0;
    virtual std::vector<KnowledgeEntity> FindByVendor(const std::string& vendor) = 0;
    virtual std::vector<KnowledgeEntity> FindByRoom(const std::string& roomId) = 0;
    virtual std::vector<KnowledgeEntity> FindActiveDevices() = 0;
    virtual std::vector<KnowledgeEntity> FindReachableDevices() = 0;
    virtual std::vector<KnowledgeEntity> FindControllerCandidates(const std::string& controllerName) = 0;
    virtual std::vector<KnowledgeEntity> FindEntitiesUsingProtocol(const std::string& protocol) = 0;
    virtual std::vector<KnowledgeEntity> GetAllEntities() = 0;

    // Graph & Relationship Operations
    virtual bool LinkEntities(const std::string& sourceId, const std::string& targetId, RelationshipType type) = 0;
    virtual std::vector<KnowledgeEntity> GetRelatedEntities(const std::string& entityId, RelationshipType type) = 0;

    // Soft Delete & Recovery
    virtual bool SoftDeleteEntity(const std::string& entityId) = 0;
    virtual bool RestoreEntity(const std::string& entityId) = 0;
    virtual bool PermanentPurge(const std::string& entityId) = 0;

    // In-Memory Flush, Synchronization & Diagnostics (Phase 2 Core)
    virtual bool LoadFromStorage() = 0;
    virtual bool FlushDirty() = 0;
    virtual bool FlushAll() = 0;
    virtual RepositoryStats GetStats() = 0;
};

} // namespace Persistence
} // namespace NetDiscovery
