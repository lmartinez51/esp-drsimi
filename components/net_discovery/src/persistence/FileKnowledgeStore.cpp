#include "FileKnowledgeStore.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "esp_log.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>

static const char* TAG = "NetDiscovery";

namespace NetDiscovery {

FileKnowledgeStore::FileKnowledgeStore(const std::string& baseDir)
    : m_baseDir(baseDir)
{
}

void FileKnowledgeStore::Initialize() {
    EnsureDirectoryExists(m_baseDir);
}

void FileKnowledgeStore::SaveEntityData(const std::string& networkId, 
                                        const std::string& entityId, 
                                        const std::string& serializedData) {
    const std::string netDir = GetNetworkDir(networkId);
    if (!EnsureDirectoryExists(netDir)) {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Failed to create network directory: %s", netDir.c_str());
        return;
    }

    const std::string filePath = GetEntityFilePath(networkId, entityId);
    FILE* f = fopen(filePath.c_str(), "wb");
    if (f) {
        size_t len = serializedData.length();
        if (len > 0) {
            // Explicitly malloc to bypass std::string SSO (Small String Optimization) 
            // which could place the string data on the PSRAM-backed stack.
            char* safe_buf = (char*)malloc(len);
            if (safe_buf) {
                memcpy(safe_buf, serializedData.data(), len);
                fwrite(safe_buf, 1, len, f);
                free(safe_buf);
            } else {
                ESP_LOGE(TAG, "[FileKnowledgeStore] Malloc failed for write buffer");
            }
        }
        fclose(f);
    } else {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Failed to open file for writing: %s", filePath.c_str());
    }
}

std::string FileKnowledgeStore::LoadEntityData(const std::string& networkId, 
                                               const std::string& entityId) {
    const std::string filePath = GetEntityFilePath(networkId, entityId);
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) {
        return "";
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return "";
    }

    // Allocate safe buffer from heap (PSRAM or Internal)
    char* safe_buf = (char*)malloc(size);
    if (!safe_buf) {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Malloc failed for read buffer");
        fclose(f);
        return "";
    }

    size_t read_bytes = fread(safe_buf, 1, size, f);
    std::string result(safe_buf, read_bytes);
    
    free(safe_buf);
    fclose(f);
    
    return result;
}

std::vector<std::string> FileKnowledgeStore::LoadAllEntities(const std::string& networkId) {
    std::vector<std::string> entities;
    const std::string netDir = GetNetworkDir(networkId);

    DIR* dir = opendir(netDir.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string entityId = entry->d_name;
                // Strip extension if it's .json
                if (entityId.size() > 5 && entityId.substr(entityId.size() - 5) == ".json") {
                    entityId = entityId.substr(0, entityId.size() - 5);
                }
                
                std::string data = LoadEntityData(networkId, entityId);
                if (!data.empty()) {
                    entities.push_back(std::move(data));
                }
            }
        }
        closedir(dir);
    } else {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Failed to open directory: %s", netDir.c_str());
    }

    return entities;
}

void FileKnowledgeStore::DeleteEntityData(const std::string& networkId, 
                                          const std::string& entityId) {
    const std::string filePath = GetEntityFilePath(networkId, entityId);
    std::remove(filePath.c_str());
}

std::string FileKnowledgeStore::GetNetworkDir(const std::string& networkId) const {
    return m_baseDir + "/" + networkId;
}

std::string FileKnowledgeStore::GetEntityFilePath(const std::string& networkId, const std::string& entityId) const {
    // Basic sanitization of entityId to avoid path traversal (replace slashes, colons)
    std::string safeId = entityId;
    for (char& c : safeId) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return GetNetworkDir(networkId) + "/" + safeId + ".json";
}

bool FileKnowledgeStore::EnsureDirectoryExists(const std::string& path) const {
    // Caller verification: FileKnowledgeStore only calls this with m_baseDir 
    // or GetNetworkDir() (e.g. "/littlefs/knowledge/SSID"). It is never called 
    // with a full file path (e.g. ending in .json).
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return true; // Directory already exists
    }
    
    // Create directory since it's now safe from Internal RAM
    if (mkdir(path.c_str(), 0777) == 0) {
        return true;
    }
    
    ESP_LOGE(TAG, "[FileKnowledgeStore] Failed to create directory %s", path.c_str());
    return false;
}

} // namespace NetDiscovery
