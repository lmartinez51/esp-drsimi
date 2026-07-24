/**
 * @file KnowledgeGraph.cpp
 * @brief Implementation of Knowledge Graph Engine for ESP-Claw Platform.
 */

#include "core/KnowledgeGraph.h"

#include <queue>
#include <stack>
#include <sstream>
#include <chrono>
#include <algorithm>

#include "esp_log.h"

static const char* TAG = "KnowledgeGraph";

namespace NetDiscovery {

KnowledgeGraph::KnowledgeGraph(std::shared_ptr<StorageEventBus> eventBus)
    : m_eventBus(eventBus)
{
    RegisterEventSubscribers();
}

void KnowledgeGraph::RegisterEventSubscribers() {
    if (!m_eventBus) return;

    // 1. Listen to EntityDeleted -> Purge edges involving entityId
    m_delSubId = m_eventBus->Subscribe(StorageEventType::EntityDeleted, [this](const StorageEvent& event) {
        RemoveEntityEdges(event.entityId);
    });

    // 2. Listen to RelationshipCreated -> Synchronize adjacency
    m_relCreatedSubId = m_eventBus->Subscribe(StorageEventType::RelationshipCreated, [this](const StorageEvent& event) {
        auto itTarget = event.metadata.find("targetId");
        auto itType = event.metadata.find("relationshipType");
        if (itTarget != event.metadata.end() && itType != event.metadata.end()) {
            CreateEdge(event.entityId, itTarget->second, itType->second);
        }
    });

    // 3. Listen to RelationshipRemoved -> Synchronize adjacency
    m_relRemovedSubId = m_eventBus->Subscribe(StorageEventType::RelationshipRemoved, [this](const StorageEvent& event) {
        auto itTarget = event.metadata.find("targetId");
        auto itType = event.metadata.find("relationshipType");
        if (itTarget != event.metadata.end() && itType != event.metadata.end()) {
            RemoveEdge(event.entityId, itTarget->second, itType->second);
        }
    });
}

bool KnowledgeGraph::CreateEdge(const std::string& sourceId, const std::string& targetId, const std::string& relType) {
    if (sourceId.empty() || targetId.empty() || relType.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_graphMutex);
        m_adjacency[sourceId][relType].insert(targetId);
        m_reverseAdjacency[targetId][relType].insert(sourceId);
    }

    PublishGraphUpdated("EdgeCreated");
    return true;
}

bool KnowledgeGraph::RemoveEdge(const std::string& sourceId, const std::string& targetId, const std::string& relType) {
    if (sourceId.empty() || targetId.empty() || relType.empty()) return false;

    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(m_graphMutex);
        auto itSrc = m_adjacency.find(sourceId);
        if (itSrc != m_adjacency.end()) {
            auto itType = itSrc->second.find(relType);
            if (itType != itSrc->second.end()) {
                removed = (itType->second.erase(targetId) > 0);
            }
        }

        auto itDst = m_reverseAdjacency.find(targetId);
        if (itDst != m_reverseAdjacency.end()) {
            auto itType = itDst->second.find(relType);
            if (itType != itDst->second.end()) {
                itType->second.erase(sourceId);
            }
        }
    }

    if (removed) {
        PublishGraphUpdated("EdgeRemoved");
    }
    return removed;
}

bool KnowledgeGraph::UpdateEdge(
    const std::string& sourceId,
    const std::string& targetId,
    const std::string& oldType,
    const std::string& newType)
{
    RemoveEdge(sourceId, targetId, oldType);
    return CreateEdge(sourceId, targetId, newType);
}

bool KnowledgeGraph::HasEdge(const std::string& sourceId, const std::string& targetId, const std::string& relType) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    auto itSrc = m_adjacency.find(sourceId);
    if (itSrc == m_adjacency.end()) return false;

    if (!relType.empty()) {
        auto itType = itSrc->second.find(relType);
        if (itType == itSrc->second.end()) return false;
        return itType->second.find(targetId) != itType->second.end();
    }

    for (const auto& [type, targets] : itSrc->second) {
        if (targets.find(targetId) != targets.end()) return true;
    }
    return false;
}

size_t KnowledgeGraph::RelationshipCount(const std::string& sourceId) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    size_t count = 0;
    auto itSrc = m_adjacency.find(sourceId);
    if (itSrc != m_adjacency.end()) {
        for (const auto& [type, targets] : itSrc->second) {
            count += targets.size();
        }
    }
    return count;
}

std::vector<std::string> KnowledgeGraph::Neighbors(const std::string& sourceId, const std::string& relType) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::vector<std::string> result;
    auto itSrc = m_adjacency.find(sourceId);
    if (itSrc == m_adjacency.end()) return result;

    if (!relType.empty()) {
        auto itType = itSrc->second.find(relType);
        if (itType != itSrc->second.end()) {
            result.assign(itType->second.begin(), itType->second.end());
        }
    } else {
        std::unordered_set<std::string> uniqueTargets;
        for (const auto& [type, targets] : itSrc->second) {
            uniqueTargets.insert(targets.begin(), targets.end());
        }
        result.assign(uniqueTargets.begin(), uniqueTargets.end());
    }
    return result;
}

std::vector<std::string> KnowledgeGraph::FindParents(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::unordered_set<std::string> parents;

    // 1. (P) -[PARENT_OF]-> (entityId)
    auto itRev = m_reverseAdjacency.find(entityId);
    if (itRev != m_reverseAdjacency.end()) {
        auto itType = itRev->second.find("PARENT_OF");
        if (itType != itRev->second.end()) {
            parents.insert(itType->second.begin(), itType->second.end());
        }
    }

    // 2. (entityId) -[CHILD_OF]-> (P)
    auto itFwd = m_adjacency.find(entityId);
    if (itFwd != m_adjacency.end()) {
        auto itType = itFwd->second.find("CHILD_OF");
        if (itType != itFwd->second.end()) {
            parents.insert(itType->second.begin(), itType->second.end());
        }
    }

    return std::vector<std::string>(parents.begin(), parents.end());
}

std::vector<std::string> KnowledgeGraph::FindChildren(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::unordered_set<std::string> children;

    // 1. (entityId) -[PARENT_OF]-> (C)
    auto itFwd = m_adjacency.find(entityId);
    if (itFwd != m_adjacency.end()) {
        auto itType = itFwd->second.find("PARENT_OF");
        if (itType != itFwd->second.end()) {
            children.insert(itType->second.begin(), itType->second.end());
        }
    }

    // 2. (C) -[CHILD_OF]-> (entityId)
    auto itRev = m_reverseAdjacency.find(entityId);
    if (itRev != m_reverseAdjacency.end()) {
        auto itType = itRev->second.find("CHILD_OF");
        if (itType != itRev->second.end()) {
            children.insert(itType->second.begin(), itType->second.end());
        }
    }

    return std::vector<std::string>(children.begin(), children.end());
}

std::vector<std::string> KnowledgeGraph::FindControllers(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::unordered_set<std::string> controllers;

    // 1. (Ctrl) -[CONTROLS]-> (entityId)
    auto itRev = m_reverseAdjacency.find(entityId);
    if (itRev != m_reverseAdjacency.end()) {
        auto itType = itRev->second.find("CONTROLS");
        if (itType != itRev->second.end()) {
            controllers.insert(itType->second.begin(), itType->second.end());
        }
    }

    // 2. (entityId) -[CONTROLLED_BY]-> (Ctrl)
    auto itFwd = m_adjacency.find(entityId);
    if (itFwd != m_adjacency.end()) {
        auto itType = itFwd->second.find("CONTROLLED_BY");
        if (itType != itFwd->second.end()) {
            controllers.insert(itType->second.begin(), itType->second.end());
        }
    }

    return std::vector<std::string>(controllers.begin(), controllers.end());
}

std::vector<std::string> KnowledgeGraph::FindControlledDevices(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::unordered_set<std::string> devices;

    // 1. (entityId) -[CONTROLS]-> (Dev)
    auto itFwd = m_adjacency.find(entityId);
    if (itFwd != m_adjacency.end()) {
        auto itType = itFwd->second.find("CONTROLS");
        if (itType != itFwd->second.end()) {
            devices.insert(itType->second.begin(), itType->second.end());
        }
    }

    // 2. (Dev) -[CONTROLLED_BY]-> (entityId)
    auto itRev = m_reverseAdjacency.find(entityId);
    if (itRev != m_reverseAdjacency.end()) {
        auto itType = itRev->second.find("CONTROLLED_BY");
        if (itType != itRev->second.end()) {
            devices.insert(itType->second.begin(), itType->second.end());
        }
    }

    return std::vector<std::string>(devices.begin(), devices.end());
}

std::vector<std::string> KnowledgeGraph::FindRoomMembers(const std::string& roomEntityId) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::unordered_set<std::string> members;

    // (E) -[LOCATED_IN]-> (roomEntityId)
    auto itRev = m_reverseAdjacency.find(roomEntityId);
    if (itRev != m_reverseAdjacency.end()) {
        auto itLoc = itRev->second.find("LOCATED_IN");
        if (itLoc != itRev->second.end()) {
            members.insert(itLoc->second.begin(), itLoc->second.end());
        }
        auto itParent = itRev->second.find("PARENT_OF");
        if (itParent != itRev->second.end()) {
            members.insert(itParent->second.begin(), itParent->second.end());
        }
    }

    return std::vector<std::string>(members.begin(), members.end());
}

std::vector<std::string> KnowledgeGraph::FindEntitiesUsingProtocol(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::vector<std::string> result;

    auto itRev = m_reverseAdjacency.find(protocol);
    if (itRev != m_reverseAdjacency.end()) {
        auto itType = itRev->second.find("USES_PROTOCOL");
        if (itType != itRev->second.end()) {
            result.assign(itType->second.begin(), itType->second.end());
        }
    }
    return result;
}

void KnowledgeGraph::TraverseBreadthFirst(const std::string& startId, TraversalCallback callback) const {
    if (!callback) return;

    std::vector<std::pair<std::string, int>> visits;

    {
        std::lock_guard<std::mutex> lock(m_graphMutex);
        std::unordered_set<std::string> visited;
        std::queue<std::pair<std::string, int>> q;

        q.push({startId, 0});
        visited.insert(startId);

        while (!q.empty()) {
            auto [currId, depth] = q.front();
            q.pop();

            visits.push_back({currId, depth});

            auto itSrc = m_adjacency.find(currId);
            if (itSrc != m_adjacency.end()) {
                for (const auto& [type, targets] : itSrc->second) {
                    for (const auto& target : targets) {
                        if (visited.find(target) == visited.end()) {
                            visited.insert(target);
                            q.push({target, depth + 1});
                        }
                    }
                }
            }
        }
    }

    // Safely invoke callbacks outside graph lock
    for (const auto& [id, depth] : visits) {
        callback(id, depth);
    }
}

void KnowledgeGraph::TraverseDepthFirst(const std::string& startId, TraversalCallback callback) const {
    if (!callback) return;

    std::vector<std::pair<std::string, int>> visits;

    {
        std::lock_guard<std::mutex> lock(m_graphMutex);
        std::unordered_set<std::string> visited;
        std::stack<std::pair<std::string, int>> st;

        st.push({startId, 0});

        while (!st.empty()) {
            auto [currId, depth] = st.top();
            st.pop();

            if (visited.find(currId) != visited.end()) continue;
            visited.insert(currId);

            visits.push_back({currId, depth});

            auto itSrc = m_adjacency.find(currId);
            if (itSrc != m_adjacency.end()) {
                for (const auto& [type, targets] : itSrc->second) {
                    for (const auto& target : targets) {
                        if (visited.find(target) == visited.end()) {
                            st.push({target, depth + 1});
                        }
                    }
                }
            }
        }
    }

    for (const auto& [id, depth] : visits) {
        callback(id, depth);
    }
}

std::vector<std::string> KnowledgeGraph::ShortestPath(const std::string& startId, const std::string& targetId) const {
    if (startId.empty() || targetId.empty()) return {};
    if (startId == targetId) return {startId};

    std::lock_guard<std::mutex> lock(m_graphMutex);

    std::unordered_set<std::string> visited;
    std::unordered_map<std::string, std::string> parentMap;
    std::queue<std::string> q;

    q.push(startId);
    visited.insert(startId);

    bool found = false;
    while (!q.empty()) {
        std::string curr = q.front();
        q.pop();

        if (curr == targetId) {
            found = true;
            break;
        }

        auto itSrc = m_adjacency.find(curr);
        if (itSrc != m_adjacency.end()) {
            for (const auto& [type, targets] : itSrc->second) {
                for (const auto& nextNode : targets) {
                    if (visited.find(nextNode) == visited.end()) {
                        visited.insert(nextNode);
                        parentMap[nextNode] = curr;
                        q.push(nextNode);
                    }
                }
            }
        }
    }

    if (!found) return {};

    // Reconstruct Path
    std::vector<std::string> path;
    std::string curr = targetId;
    while (curr != startId) {
        path.push_back(curr);
        curr = parentMap[curr];
    }
    path.push_back(startId);
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::string> KnowledgeGraph::FindReachable(const std::string& startId) const {
    std::vector<std::string> reachable;
    TraverseBreadthFirst(startId, [&](const std::string& id, int depth) {
        reachable.push_back(id);
    });
    return reachable;
}

void KnowledgeGraph::RebuildFromEntities(const std::vector<KnowledgeEntity>& entities) {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    m_adjacency.clear();
    m_reverseAdjacency.clear();

    size_t edgeCount = 0;
    for (const auto& entity : entities) {
        if (entity.persistentId.empty() || entity.lifecycle.userDeleted) continue;

        // 1. Explicit Graph Relationships
        for (const auto& rel : entity.relationships) {
            if (!rel.targetId.empty() && !rel.type.empty()) {
                m_adjacency[entity.persistentId][rel.type].insert(rel.targetId);
                m_reverseAdjacency[rel.targetId][rel.type].insert(entity.persistentId);
                edgeCount++;
            }
        }

        // 2. Implicit Controllers
        for (const auto& ctrl : entity.compatibleControllers) {
            if (!ctrl.name.empty()) {
                m_adjacency[entity.persistentId]["CONTROLS"].insert(ctrl.name);
                m_reverseAdjacency[ctrl.name]["CONTROLS"].insert(entity.persistentId);
                edgeCount++;
            }
        }

        // 3. Implicit Protocols
        for (const auto& ep : entity.endpoints) {
            if (!ep.serverHeader.empty()) {
                m_adjacency[entity.persistentId]["USES_PROTOCOL"].insert(ep.serverHeader);
                m_reverseAdjacency[ep.serverHeader]["USES_PROTOCOL"].insert(entity.persistentId);
                edgeCount++;
            }
        }
    }

    ESP_LOGI(TAG, "Rebuilt KnowledgeGraph adjacency in O(N+E): %zu nodes, %zu edges", entities.size(), edgeCount);
    PublishGraphUpdated("RebuildFromEntities");
}

void KnowledgeGraph::RemoveEntityEdges(const std::string& entityId) {
    if (entityId.empty()) return;

    {
        std::lock_guard<std::mutex> lock(m_graphMutex);

        // Remove forward edges from entityId
        auto itSrc = m_adjacency.find(entityId);
        if (itSrc != m_adjacency.end()) {
            for (const auto& [type, targets] : itSrc->second) {
                for (const auto& target : targets) {
                    auto itRev = m_reverseAdjacency.find(target);
                    if (itRev != m_reverseAdjacency.end()) {
                        itRev->second[type].erase(entityId);
                    }
                }
            }
            m_adjacency.erase(itSrc);
        }

        // Remove reverse edges targeting entityId
        auto itRev = m_reverseAdjacency.find(entityId);
        if (itRev != m_reverseAdjacency.end()) {
            for (const auto& [type, sources] : itRev->second) {
                for (const auto& src : sources) {
                    auto itFwd = m_adjacency.find(src);
                    if (itFwd != m_adjacency.end()) {
                        itFwd->second[type].erase(entityId);
                    }
                }
            }
            m_reverseAdjacency.erase(itRev);
        }
    }

    PublishGraphUpdated("EntityEdgesRemoved");
}

std::string KnowledgeGraph::ExportAdjacencyList() const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::stringstream ss;
    for (const auto& [src, rels] : m_adjacency) {
        ss << src << ":\n";
        for (const auto& [relType, targets] : rels) {
            ss << "  - [" << relType << "] -> ";
            for (const auto& dst : targets) {
                ss << dst << " ";
            }
            ss << "\n";
        }
    }
    return ss.str();
}

std::string KnowledgeGraph::ExportDOT() const {
    std::lock_guard<std::mutex> lock(m_graphMutex);
    std::stringstream ss;
    ss << "digraph KnowledgeGraph {\n";
    ss << "    rankdir=LR;\n";
    ss << "    node [shape=box, style=\"rounded,filled\", fillcolor=\"#F4F6F7\", fontname=\"Helvetica\"];\n";
    ss << "    edge [fontname=\"Helvetica\", fontsize=10];\n\n";

    for (const auto& [src, rels] : m_adjacency) {
        for (const auto& [relType, targets] : rels) {
            for (const auto& dst : targets) {
                ss << "    \"" << src << "\" -> \"" << dst << "\" [label=\"" << relType << "\"];\n";
            }
        }
    }

    ss << "}\n";
    return ss.str();
}

void KnowledgeGraph::PublishGraphUpdated(const std::string& reason) {
    if (!m_eventBus) return;

    using namespace std::chrono;
    uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    StorageEvent event;
    event.type = StorageEventType::GraphUpdated;
    event.timestamp = now;
    event.metadata["reason"] = reason;

    m_eventBus->Publish(event);
}

} // namespace NetDiscovery
