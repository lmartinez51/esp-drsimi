/**
 * @file KnowledgeStore.h
 * @brief Owns knowledge merging, state computation, and interaction with persistence.
 */

#pragma once

#include "persistence/IKnowledgeStore.h"
#include "core/KnowledgeEntity.h"
#include "core/NetworkFingerprint.h"
#include "core/LogicalDevice.h"

#include <memory>
#include <vector>
#include <string>
#include <map>

namespace NetDiscovery {

/**
 * @brief Central authority for the application's cognitive state.
 * Contains merge policies, journaling, and confidence computation.
 */
class KnowledgeStore {
public:
    explicit KnowledgeStore(std::unique_ptr<IKnowledgeStore> backend);

    /**
     * @brief Initializes the underlying storage.
     */
    void Initialize();

    /**
     * @brief Loads all knowledge for a specific network fingerprint.
     */
    void ResolveKnownNetwork(const NetworkFingerprint& network);

    /**
     * @brief Merges a live discovery result into the persistent knowledge.
     * Live observations always take precedence over persisted knowledge.
     */
    void UpdateFromDiscovery(const LogicalDevice& liveDevice);

    /**
     * @brief Marks an entity as archived (replaces destructive Forget).
     */
    void ArchiveEntity(const std::string& entityId);

    /**
     * @brief Mark an entity for update with new credentials.
     */
    void UpdateCredentials(const std::string& deviceId, const std::string& key, const std::string& value);

    /**
     * @brief Directly appends a communication record (called by Synchronizer).
     */
    void AppendCommunicationRecord(const std::string& entityId, const CommunicationRecord& record);

    /**
     * @brief Computes runtime confidence state for a given entity.
     */
    KnowledgeConfidence ComputeConfidence(const KnowledgeEntity& entity) const;

    /**
     * @brief Retrieve all entities currently loaded in memory.
     */
    std::vector<KnowledgeEntity>& GetLoadedEntities();

private:
    std::unique_ptr<IKnowledgeStore> m_backend;
    NetworkFingerprint m_currentNetwork;
    std::map<std::string, KnowledgeEntity> m_entities; // In-memory cache of current network

    // Internal serialization helpers (simple key-value text format)
    std::string SerializeEntity(const KnowledgeEntity& entity) const;
    KnowledgeEntity DeserializeEntity(const std::string& data) const;
    
    // Internal Merge Logic
    void MergeEndpoints(KnowledgeEntity& existing, const std::vector<ProtocolEndpoint>& liveEndpoints);
    void MergeCapabilities(KnowledgeEntity& existing, const std::vector<Capability>& liveCaps);
    void MergeCapabilityProfiles(KnowledgeEntity& existing, const std::vector<CapabilityProfile>& liveProfiles);
    void AddJournalEntry(KnowledgeEntity& entity, JournalEventType type, const std::string& description);
    
    void PersistEntity(const KnowledgeEntity& entity);
};

} // namespace NetDiscovery
