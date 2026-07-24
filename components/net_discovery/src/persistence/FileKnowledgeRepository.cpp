/**
 * @file FileKnowledgeRepository.cpp
 * @brief File-backed implementation of IKnowledgeRepository.
 * Manages ESP-Claw persistent knowledge entities without directly knowing JSON mechanics.
 */

#include "persistence/FileKnowledgeRepository.h"
#include "persistence/AtomicFileWriter.h"
#include "persistence/JsonKnowledgeSerializer.h"

#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "esp_log.h"

static const char* TAG = "FileKnowledgeRepo";

namespace NetDiscovery {
namespace Persistence {

FileKnowledgeRepository::FileKnowledgeRepository(
    std::shared_ptr<RepositoryContext> context,
    std::unique_ptr<IKnowledgeSerializer> serializer)
    : m_context(context ? context : std::make_shared<RepositoryContext>()),
      m_serializer(serializer ? std::move(serializer) : std::make_unique<JsonKnowledgeSerializer>())
{
    m_storageDir = m_context->GetKnowledgePath();
    EnsureDirectoryExists();
}

void FileKnowledgeRepository::EnsureDirectoryExists() {
    struct stat st;
    if (stat(m_context->GetBasePath().c_str(), &st) != 0) {
        mkdir(m_context->GetBasePath().c_str(), 0755);
    }
    if (stat(m_storageDir.c_str(), &st) != 0) {
        mkdir(m_storageDir.c_str(), 0755);
    }
}

std::string FileKnowledgeRepository::GetEntityFilePath(const std::string& entityId) const {
    std::string cleanId = entityId;
    for (char& c : cleanId) {
        if (c == ':' || c == '/' || c == '\\' || c == ' ') c = '_';
    }
    return m_storageDir + "/" + cleanId + ".json";
}

void FileKnowledgeRepository::ReloadCacheLocked() {
    if (m_cacheLoaded) return;
    m_cache.clear();

    DIR* dir = opendir(m_storageDir.c_str());
    if (!dir) {
        m_cacheLoaded = true;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            std::string filename = entry->d_name;
            if (filename.length() > 5 && filename.substr(filename.length() - 5) == ".json") {
                std::string fullPath = m_storageDir + "/" + filename;
                std::ifstream f(fullPath);
                if (f.is_open()) {
                    std::stringstream buffer;
                    buffer << f.rdbuf();
                    std::string rawData = buffer.str();
                    auto entityOpt = m_serializer->Deserialize(rawData);
                    if (entityOpt.has_value()) {
                        m_cache[entityOpt->persistentId] = entityOpt.value();
                    }
                }
            }
        }
    }
    closedir(dir);
    m_cacheLoaded = true;
}

bool FileKnowledgeRepository::SaveEntity(const KnowledgeEntity& entity) {
    if (entity.persistentId.empty()) {
        ESP_LOGE(TAG, "Cannot save entity with empty persistentId");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::string serialized = m_serializer->Serialize(entity);
    if (serialized.empty()) {
        ESP_LOGE(TAG, "Failed to serialize entity %s", entity.persistentId.c_str());
        return false;
    }

    std::string filePath = GetEntityFilePath(entity.persistentId);
    bool ok = AtomicFileWriter::WriteStringAtomic(filePath, serialized);
    if (ok) {
        m_cache[entity.persistentId] = entity;
        ESP_LOGI(TAG, "Saved entity %s atomically", entity.persistentId.c_str());
    } else {
        ESP_LOGE(TAG, "Failed to write entity file %s", filePath.c_str());
    }
    return ok;
}

std::optional<KnowledgeEntity> FileKnowledgeRepository::FindById(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    auto it = m_cache.find(entityId);
    if (it != m_cache.end() && !it->second.lifecycle.userDeleted) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<KnowledgeEntity> FileKnowledgeRepository::FindByMac(const std::string& macAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    for (const auto& pair : m_cache) {
        if (!pair.second.lifecycle.userDeleted && pair.second.identity.macAddress == macAddress) {
            return pair.second;
        }
    }
    return std::nullopt;
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::GetAllEntities() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_cache) {
        if (!pair.second.lifecycle.userDeleted) {
            res.push_back(pair.second);
        }
    }
    return res;
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::FindByCapability(const std::string& capability) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_cache) {
        if (pair.second.lifecycle.userDeleted) continue;
        for (const auto& cap : pair.second.capabilities.GetCapabilities()) {
            if (cap.id == capability || NetDiscovery::ToString(cap) == capability) {
                res.push_back(pair.second);
                break;
            }
        }
    }
    return res;
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::FindByVendor(const std::string& vendor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_cache) {
        if (!pair.second.lifecycle.userDeleted && pair.second.identity.vendor == vendor) {
            res.push_back(pair.second);
        }
    }
    return res;
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::FindByRoom(const std::string& roomEntityId) {
    return GetRelatedEntities(roomEntityId, RelationshipType::LocatedIn);
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::FindActiveDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_cache) {
        if (!pair.second.lifecycle.userDeleted && pair.second.lifecycle.state == "ACTIVE") {
            res.push_back(pair.second);
        }
    }
    return res;
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::FindReachableDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_cache) {
        if (!pair.second.lifecycle.userDeleted && pair.second.runtimeState.isOnline) {
            res.push_back(pair.second);
        }
    }
    return res;
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::FindControllerCandidates(const std::string& capabilityName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_cache) {
        if (pair.second.lifecycle.userDeleted) continue;
        for (const auto& ctrl : pair.second.compatibleControllers) {
            if (ctrl.name == capabilityName) {
                res.push_back(pair.second);
                break;
            }
        }
    }
    return res;
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::FindEntitiesUsingProtocol(const std::string& protocol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    for (const auto& pair : m_cache) {
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

bool FileKnowledgeRepository::LinkEntities(const std::string& sourceId, const std::string& targetId, RelationshipType type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    auto it = m_cache.find(sourceId);
    if (it == m_cache.end()) return false;

    EntityRelationship rel;
    rel.type = ToString(type);
    rel.targetId = targetId;

    it->second.relationships.push_back(rel);
    return SaveEntity(it->second);
}

std::vector<KnowledgeEntity> FileKnowledgeRepository::GetRelatedEntities(const std::string& entityId, RelationshipType type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    std::vector<KnowledgeEntity> res;
    auto it = m_cache.find(entityId);
    if (it != m_cache.end()) {
        for (const auto& rel : it->second.relationships) {
            if (rel.type == ToString(type)) {
                auto targetIt = m_cache.find(rel.targetId);
                if (targetIt != m_cache.end() && !targetIt->second.lifecycle.userDeleted) {
                    res.push_back(targetIt->second);
                }
            }
        }
    }
    return res;
}

bool FileKnowledgeRepository::SoftDeleteEntity(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    auto it = m_cache.find(entityId);
    if (it == m_cache.end()) return false;

    it->second.lifecycle.userDeleted = true;
    it->second.lifecycle.state = "SOFT_DELETED";
    return SaveEntity(it->second);
}

bool FileKnowledgeRepository::RestoreEntity(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    auto it = m_cache.find(entityId);
    if (it == m_cache.end()) return false;

    it->second.lifecycle.userDeleted = false;
    it->second.lifecycle.state = "ACTIVE";
    return SaveEntity(it->second);
}

bool FileKnowledgeRepository::PermanentPurge(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReloadCacheLocked();

    auto it = m_cache.find(entityId);
    if (it == m_cache.end()) return false;

    std::string filePath = GetEntityFilePath(entityId);
    remove(filePath.c_str());
    m_cache.erase(it);
    ESP_LOGI(TAG, "Permanently purged entity %s", entityId.c_str());
    return true;
}

} // namespace Persistence
} // namespace NetDiscovery
