/**
 * @file KnowledgeGraph.h
 * @brief Knowledge Graph Engine for ESP-Claw Platform (v5.0.0 Architecture).
 * Manages relationships, adjacencies, graph traversals, and topology events independently of node persistent storage.
 */

#pragma once

#include "core/KnowledgeEntity.h"
#include "core/StorageEventBus.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <memory>

namespace NetDiscovery {

/**
 * @brief Represents a single directed edge in the Knowledge Graph.
 */
struct GraphEdge {
    std::string sourceId;
    std::string targetId;
    std::string type;
};

/**
 * @brief Dedicated Graph Engine managing entity adjacencies, traversals, and topology events.
 */
class KnowledgeGraph {
public:
    explicit KnowledgeGraph(std::shared_ptr<StorageEventBus> eventBus = nullptr);
    ~KnowledgeGraph() = default;

    // Non-copyable, non-movable
    KnowledgeGraph(const KnowledgeGraph&) = delete;
    KnowledgeGraph& operator=(const KnowledgeGraph&) = delete;

    // Edge Management API
    bool CreateEdge(const std::string& sourceId, const std::string& targetId, const std::string& relType);
    bool RemoveEdge(const std::string& sourceId, const std::string& targetId, const std::string& relType);
    bool UpdateEdge(const std::string& sourceId, const std::string& targetId, const std::string& oldType, const std::string& newType);
    
    // Synonyms matching architecture spec
    bool Connect(const std::string& sourceId, const std::string& targetId, const std::string& relType) {
        return CreateEdge(sourceId, targetId, relType);
    }
    bool Disconnect(const std::string& sourceId, const std::string& targetId, const std::string& relType) {
        return RemoveEdge(sourceId, targetId, relType);
    }
    bool HasEdge(const std::string& sourceId, const std::string& targetId, const std::string& relType = "") const;
    bool HasRelationship(const std::string& sourceId, const std::string& targetId, const std::string& relType = "") const {
        return HasEdge(sourceId, targetId, relType);
    }

    size_t RelationshipCount(const std::string& sourceId) const;
    size_t Degree(const std::string& sourceId) const { return RelationshipCount(sourceId); }

    // Neighborhood & Relationship Queries
    std::vector<std::string> Neighbors(const std::string& sourceId, const std::string& relType = "") const;
    std::vector<std::string> FindNeighbors(const std::string& sourceId, const std::string& relType = "") const {
        return Neighbors(sourceId, relType);
    }
    std::vector<std::string> FindParents(const std::string& entityId) const;
    std::vector<std::string> FindChildren(const std::string& entityId) const;
    std::vector<std::string> FindControllers(const std::string& entityId) const;
    std::vector<std::string> FindControlledDevices(const std::string& entityId) const;
    std::vector<std::string> FindRoomMembers(const std::string& roomEntityId) const;
    std::vector<std::string> FindEntitiesUsingProtocol(const std::string& protocol) const;

    // Graph Traversal Algorithms
    using TraversalCallback = std::function<void(const std::string& entityId, int depth)>;
    void TraverseBreadthFirst(const std::string& startId, TraversalCallback callback) const;
    void TraverseDepthFirst(const std::string& startId, TraversalCallback callback) const;
    std::vector<std::string> ShortestPath(const std::string& startId, const std::string& targetId) const;
    std::vector<std::string> FindShortestPath(const std::string& startId, const std::string& targetId) const {
        return ShortestPath(startId, targetId);
    }
    std::vector<std::string> FindReachable(const std::string& startId) const;

    // Startup & Batch Operations
    void RebuildFromEntities(const std::vector<KnowledgeEntity>& entities);
    void RemoveEntityEdges(const std::string& entityId);

    // Export Utilities
    std::string ExportAdjacencyList() const;
    std::string ExportDOT() const;

private:
    void PublishGraphUpdated(const std::string& reason);
    void RegisterEventSubscribers();

    std::shared_ptr<StorageEventBus> m_eventBus;
    StorageEventBus::SubscriptionId m_delSubId{0};
    StorageEventBus::SubscriptionId m_relCreatedSubId{0};
    StorageEventBus::SubscriptionId m_relRemovedSubId{0};

    mutable std::mutex m_graphMutex;

    // Forward Adjacency: sourceId -> relType -> unordered_set<targetId>
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_set<std::string>>> m_adjacency;

    // Reverse Adjacency: targetId -> relType -> unordered_set<sourceId>
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_set<std::string>>> m_reverseAdjacency;
};

} // namespace NetDiscovery
