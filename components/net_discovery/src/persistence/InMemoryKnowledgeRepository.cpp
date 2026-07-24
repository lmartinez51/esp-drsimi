/**
 * @file InMemoryKnowledgeRepository.cpp
 * @brief Implementation of High-Performance In-Memory Knowledge Repository with Multi-Indexing and Batched Atomic Flushes.
 */

#include "persistence/InMemoryKnowledgeRepository.h"
#include "persistence/AtomicFileWriter.h"
#include "persistence/JsonKnowledgeSerializer.h"

#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "esp_log.h"

static const char* TAG = "InMemoryKnowledgeRepo";

namespace NetDiscovery {
namespace Persistence {

InMemoryKnowledgeRepository::InMemoryKnowledgeRepository(
    std::shared_ptr<RepositoryContext> context,
    std::unique_ptr<IKnowledgeSerializer> serializer,
    std::shared_ptr<StorageEventBus> eventBus)
    : m_context(context ? context : std::make_shared<RepositoryContext>()),
      m_serializer(serializer ? std::move(serializer) : std::make_unique<JsonKnowledgeSerializer>()),
      m_eventBus(eventBus)
{
    m_storageDir = m_context->GetKnowledgePath();
    EnsureDirectoryExists();
    LoadFromStorage();
}

InMemoryKnowledgeRepository::~InMemoryKnowledgeRepository() {
    FlushDirty();
}

void InMemoryKnowledgeRepository::EnsureDirectoryExists() {
    struct stat st;
    if (stat(m_context->GetBasePath().c_str(), &st) != 0) {
        mkdir(m_context->GetBasePath().c_str(), 0755);
    }
    if (stat(m_storageDir.c_str(), &st) != 0) {
        mkdir(m_storageDir.c_str(), 0755);
    }
}

std::string InMemoryKnowledgeRepository::GetEntityFilePath(const std::string& entityId) const {
    std::string cleanId = entityId;
    for (char& c : cleanId) {
        if (c == ':' || c == '/' || c == '\\' || c == ' ') c = '_';
    }
    return m_storageDir + "/" + cleanId + ".json";
}

void InMemoryKnowledgeRepository::BuildIndexesForEntityLocked(const KnowledgeEntity& entity) {
    const std::string& id = entity.persistentId;

    // 1. MAC Index
    if (!entity.identity.macAddress.empty()) {
        m_macIndex[entity.identity.macAddress] = id;
    }

    // 2. Vendor Index
    if (!entity.identity.vendor.empty()) {
        auto& list = m_vendorIndex[entity.identity.vendor];
        if (std::find(list.begin(), list.end(), id) == list.end()) {
            list.push_back(id);
        }
    }

    // 3. Capability Index
    for (const auto& cap : entity.capabilities.GetCapabilities()) {
        const std::string& capName = cap.id;
        auto& list = m_capabilityIndex[capName];
        if (std::find(list.begin(), list.end(), id) == list.end()) {
            list.push_back(id);
        }
    }

    // 4. Controller Index
    for (const auto& ctrl : entity.compatibleControllers) {
        if (!ctrl.name.empty()) {
            auto& list = m_controllerIndex[ctrl.name];
            if (std::find(list.begin(), list.end(), id) == list.end()) {
                list.push_back(id);
            }
        }
    }

    // 5. Room Index
    for (const auto& rel : entity.relationships) {
        if (rel.type == ToString(RelationshipType::LocatedIn) && !rel.targetId.empty()) {
            auto& list = m_roomIndex[rel.targetId];
            if (std::find(list.begin(), list.end(), id) == list.end()) {
                list.push_back(id);
            }
        }
    }
}

void InMemoryKnowledgeRepository::RemoveIndexesForEntityLocked(const KnowledgeEntity& entity) {
    const std::string& id = entity.persistentId;

    if (!entity.identity.macAddress.empty()) {
        m_macIndex.erase(entity.identity.macAddress);
    }

    if (!entity.identity.vendor.empty()) {
        auto it = m_vendorIndex.find(entity.identity.vendor);
        if (it != m_vendorIndex.end()) {
            auto& list = it->second;
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
            if (list.empty()) m_vendorIndex.erase(it);
        }
    }

    for (const auto& cap : entity.capabilities.GetCapabilities()) {
        const std::string& capName = cap.id;
        auto it = m_capabilityIndex.find(capName);
        if (it != m_capabilityIndex.end()) {
            auto& list = it->second;
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
            if (list.empty()) m_capabilityIndex.erase(it);
        }
    }

    for (const auto& ctrl : entity.compatibleControllers) {
        auto it = m_controllerIndex.find(ctrl.name);
        if (it != m_controllerIndex.end()) {
            auto& list = it->second;
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
            if (list.empty()) m_controllerIndex.erase(it);
        }
    }

    for (const auto& rel : entity.relationships) {
        if (rel.type == ToString(RelationshipType::LocatedIn)) {
            auto it = m_roomIndex.find(rel.targetId);
            if (it != m_roomIndex.end()) {
                auto& list = it->second;
                list.erase(std::remove(list.begin(), list.end(), id), list.end());
                if (list.empty()) m_roomIndex.erase(it);
            }
        }
    }
}

void InMemoryKnowledgeRepository::RebuildAllIndexesLocked() {
    m_macIndex.clear();
    m_vendorIndex.clear();
    m_capabilityIndex.clear();
    m_controllerIndex.clear();
    m_roomIndex.clear();

    for (const auto& pair : m_entities) {
        if (!pair.second.lifecycle.userDeleted) {
            BuildIndexesForEntityLocked(pair.second);
        }
    }
}

bool InMemoryKnowledgeRepository::LoadFromStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entities.clear();
    m_dirtyEntityIds.clear();

    DIR* dir = opendir(m_storageDir.c_str());
    if (!dir) {
        m_isLoaded = true;
        return true;
    }

    size_t loadedCount = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 5 && filename.substr(filename.length() - 5) == ".json") {
            std::string fullPath = m_storageDir + "/" + filename;
            std::ifstream f(fullPath);
            if (f.is_open()) {
                std::stringstream buffer;
                buffer << f.rdbuf();
                std::string rawData = buffer.str();
                auto entityOpt = m_serializer->Deserialize(rawData);
                if (entityOpt.has_value() && !entityOpt->persistentId.empty()) {
                    m_entities[entityOpt->persistentId] = entityOpt.value();
                    loadedCount++;
                }
            }
        }
    }
    closedir(dir);

    RebuildAllIndexesLocked();
    m_isLoaded = true;
    ESP_LOGI(TAG, "Loaded %zu knowledge entities into memory and built multi-indexes", loadedCount);
    return true;
}

bool InMemoryKnowledgeRepository::FlushDirty() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_dirtyEntityIds.empty()) return true;

    size_t successCount = 0;
    std::vector<std::string> flushedIds;

    for (const auto& id : m_dirtyEntityIds) {
        auto it = m_entities.find(id);
        if (it != m_entities.end()) {
            std::string serialized = m_serializer->Serialize(it->second);
            if (!serialized.empty()) {
                std::string path = GetEntityFilePath(id);
                if (AtomicFileWriter::WriteStringAtomic(path, serialized)) {
                    flushedIds.push_back(id);
                    successCount++;
                }
            }
        }
    }

    for (const auto& id : flushedIds) {
        m_dirtyEntityIds.erase(id);
    }

    m_lastFlushTimestamp = m_context->GetCurrentTimestamp();
    ESP_LOGI(TAG, "Flushed %zu dirty entities to LittleFS storage", successCount);
    return (successCount == flushedIds.size());
}

bool InMemoryKnowledgeRepository::FlushAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& pair : m_entities) {
        m_dirtyEntityIds.insert(pair.first);
    }
    return FlushDirty();
}

RepositoryStats InMemoryKnowledgeRepository::GetStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    RepositoryStats stats;
    stats.totalEntities = m_entities.size();
    stats.dirtyCount = m_dirtyEntityIds.size();
    stats.indexedMacs = m_macIndex.size();
    stats.indexedVendors = m_vendorIndex.size();
    stats.indexedCapabilities = m_capabilityIndex.size();
    stats.indexedControllers = m_controllerIndex.size();
    stats.indexedRooms = m_roomIndex.size();
    stats.lastFlushTimestamp = m_lastFlushTimestamp;

    for (const auto& pair : m_entities) {
        if (pair.second.lifecycle.userDeleted) {
            stats.softDeletedEntities++;
        } else {
            stats.activeEntities++;
        }
    }
    return stats;
}

bool InMemoryKnowledgeRepository::SaveEntity(const KnowledgeEntity& entity) {
    if (entity.persistentId.empty()) {
        ESP_LOGE(TAG, "Cannot save entity with empty persistentId");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto oldIt = m_entities.find(entity.persistentId);
    if (oldIt != m_entities.end()) {
        RemoveIndexesForEntityLocked(oldIt->second);
    }

    m_entities[entity.persistentId] = entity;

    if (!entity.lifecycle.userDeleted) {
        BuildIndexesForEntityLocked(entity);
    }

    m_dirtyEntityIds.insert(entity.persistentId);
    return true;
}

std::optional<KnowledgeEntity> InMemoryKnowledgeRepository::FindById(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entities.find(entityId);
    if (it != m_entities.end() && !it->second.lifecycle.userDeleted) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<KnowledgeEntity> InMemoryKnowledgeRepository::FindByMac(const std::string& macAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_macIndex.find(macAddress);
    if (it != m_macIndex.end()) {
        auto entityIt = m_entities.find(it->second);
        if (entityIt != m_entities.end() && !entityIt->second.lifecycle.userDeleted) {
            return entityIt->second;
        }
    }
    return std::nullopt;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::FindByCapability(const std::string& capability) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    auto it = m_capabilityIndex.find(capability);
    if (it != m_capabilityIndex.end()) {
        for (const auto& id : it->second) {
            auto entityIt = m_entities.find(id);
            if (entityIt != m_entities.end() && !entityIt->second.lifecycle.userDeleted) {
                res.push_back(entityIt->second);
            }
        }
    }
    return res;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::FindByVendor(const std::string& vendor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    auto it = m_vendorIndex.find(vendor);
    if (it != m_vendorIndex.end()) {
        for (const auto& id : it->second) {
            auto entityIt = m_entities.find(id);
            if (entityIt != m_entities.end() && !entityIt->second.lifecycle.userDeleted) {
                res.push_back(entityIt->second);
            }
        }
    }
    return res;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::FindByRoom(const std::string& roomEntityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    auto it = m_roomIndex.find(roomEntityId);
    if (it != m_roomIndex.end()) {
        for (const auto& id : it->second) {
            auto entityIt = m_entities.find(id);
            if (entityIt != m_entities.end() && !entityIt->second.lifecycle.userDeleted) {
                res.push_back(entityIt->second);
            }
        }
    }
    return res;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::FindActiveDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_entities) {
        if (!pair.second.lifecycle.userDeleted && pair.second.lifecycle.state == "ACTIVE") {
            res.push_back(pair.second);
        }
    }
    return res;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::FindReachableDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_entities) {
        if (!pair.second.lifecycle.userDeleted && pair.second.runtimeState.isOnline) {
            res.push_back(pair.second);
        }
    }
    return res;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::FindControllerCandidates(const std::string& controllerName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    auto it = m_controllerIndex.find(controllerName);
    if (it != m_controllerIndex.end()) {
        for (const auto& id : it->second) {
            auto entityIt = m_entities.find(id);
            if (entityIt != m_entities.end() && !entityIt->second.lifecycle.userDeleted) {
                res.push_back(entityIt->second);
            }
        }
    }
    return res;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::FindEntitiesUsingProtocol(const std::string& protocol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_entities) {
        if (pair.second.lifecycle.userDeleted) continue;
        for (const auto& ep : pair.second.endpoints) {
            if (ep.serverHeader.find(protocol) != std::string::npos || ep.uuid == protocol) {
                res.push_back(pair.second);
                break;
            }
        }
    }
    return res;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::GetAllEntities() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_entities) {
        if (!pair.second.lifecycle.userDeleted) {
            res.push_back(pair.second);
        }
    }
    return res;
}

bool InMemoryKnowledgeRepository::LinkEntities(const std::string& sourceId, const std::string& targetId, RelationshipType type) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entities.find(sourceId);
    if (it == m_entities.end()) return false;

    EntityRelationship rel;
    rel.type = ToString(type);
    rel.targetId = targetId;

    RemoveIndexesForEntityLocked(it->second);
    it->second.relationships.push_back(rel);
    BuildIndexesForEntityLocked(it->second);

    m_dirtyEntityIds.insert(sourceId);

    if (m_eventBus) {
        StorageEvent event;
        event.type = StorageEventType::RelationshipCreated;
        event.entityId = sourceId;
        event.metadata["targetId"] = targetId;
        event.metadata["relationshipType"] = rel.type;
        m_eventBus->Publish(event);
    }
    return true;
}

std::vector<KnowledgeEntity> InMemoryKnowledgeRepository::GetRelatedEntities(const std::string& entityId, RelationshipType type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<KnowledgeEntity> res;
    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) {
        std::string relTypeStr = ToString(type);
        for (const auto& rel : it->second.relationships) {
            if (rel.type == relTypeStr) {
                auto targetIt = m_entities.find(rel.targetId);
                if (targetIt != m_entities.end() && !targetIt->second.lifecycle.userDeleted) {
                    res.push_back(targetIt->second);
                }
            }
        }
    }
    return res;
}

bool InMemoryKnowledgeRepository::SoftDeleteEntity(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return false;

    RemoveIndexesForEntityLocked(it->second);
    it->second.lifecycle.userDeleted = true;
    it->second.lifecycle.state = "SOFT_DELETED";

    m_dirtyEntityIds.insert(entityId);
    return true;
}

bool InMemoryKnowledgeRepository::RestoreEntity(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return false;

    it->second.lifecycle.userDeleted = false;
    it->second.lifecycle.state = "ACTIVE";
    BuildIndexesForEntityLocked(it->second);

    m_dirtyEntityIds.insert(entityId);
    return true;
}

bool InMemoryKnowledgeRepository::PermanentPurge(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return false;

    RemoveIndexesForEntityLocked(it->second);
    std::string filePath = GetEntityFilePath(entityId);
    remove(filePath.c_str());

    m_entities.erase(it);
    m_dirtyEntityIds.erase(entityId);
    ESP_LOGI(TAG, "Permanently purged entity %s from memory and storage", entityId.c_str());
    return true;
}

} // namespace Persistence
} // namespace NetDiscovery
