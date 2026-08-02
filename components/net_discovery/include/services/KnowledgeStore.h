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
#include <unordered_set>

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
     * @brief Post-discovery active consolidation: merges any remaining duplicate entities
     *        sharing the same IP/MAC in memory and purges redundant LittleFS files.
     */
    void Consolidate();

    /**
     * @brief Marks an entity as archived (replaces destructive Forget).
     */
    void ArchiveEntity(const std::string& entityId);

    /**
     * @brief Returns the calculated ID of the currently loaded network fingerprint.
     *        Used by external components to reconstruct LittleFS paths without hardcoding SSIDs.
     */
    std::string GetCurrentNetworkId() const { return m_currentNetwork.CalculateId(); }

    /**
     * @brief Completely removes an entity from memory and persistent storage.
     */
    bool RemoveEntity(const std::string& entityId);

    /**
     * @brief Zero-copy administrative lookup: searches m_entities in-place by IP, UUID, or
     *        case-insensitive substring on displayName/vendor/model. No heap allocation on caller side.
     * @param targetLower Null-terminated lowercase target string.
     * @param outId       Receives the canonical persistentId of the matched entity.
     * @param outDisplay  Receives the displayName of the matched entity.
     * @return  1  exactly one match found (outId/outDisplay populated)
     *          0  no match found
     *         >1  ambiguous (multiple matches found, outId/outDisplay empty)
     */
    int FindEntityForAdmin(const char* targetLower, std::string& outId, std::string& outDisplay) const;

    /**
     * @brief Formats all loaded network entities into a lightweight JSON array in-place.
     *        Zero-copy on entity structures, suitable for stack buffers in AdminPipeline.
     * @param buf Target output buffer.
     * @param bufSize Maximum size of output buffer.
     * @return Number of entities formatted into JSON array.
     */
    int FormatEntityListJson(char* buf, size_t bufSize) const;

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
     * @brief Retrieve a snapshot of all entities currently loaded in memory.
     * Returns by value — thread-safe, no dangling references across FreeRTOS tasks.
     */
    std::vector<KnowledgeEntity> GetLoadedEntities() const;

private:
    std::unique_ptr<IKnowledgeStore> m_backend;
    NetworkFingerprint m_currentNetwork;
    std::map<std::string, KnowledgeEntity> m_entities; // In-memory cache of current network
    std::unordered_set<std::string> m_tombstones; // Session-only forgotten entity tombstone IDs

    // Internal serialization helpers (simple key-value text format)
    std::string SerializeEntity(const KnowledgeEntity& entity) const;
    KnowledgeEntity DeserializeEntity(const std::string& data) const;
    
    // Internal Merge Logic
    void MergeEndpoints(KnowledgeEntity& existing, const std::vector<ProtocolEndpoint>& liveEndpoints);
    void MergeCapabilities(KnowledgeEntity& existing, const std::vector<Capability>& liveCaps);
    void MergeCapabilityProfiles(KnowledgeEntity& existing, const std::vector<CapabilityProfile>& liveProfiles);
    void AddJournalEntry(KnowledgeEntity& entity, JournalEventType type, const std::string& description);

    /**
     * @brief Boot-time self-healing pass: detect entities sharing the same IP or MAC
     *        and merge them into a single canonical entry, purging redundant LittleFS files.
     */
    void ConsolidateDuplicates(const std::string& networkId);

    void PersistEntity(const KnowledgeEntity& entity);
};

} // namespace NetDiscovery
