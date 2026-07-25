#include "FileKnowledgeStore.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "NetDiscoveryIPC.h"
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
    const size_t len = serializedData.length();
    if (len == 0) {
        return;
    }

    // Fix 3: this may run on a PSRAM-backed stack (nd_oneshot), so no direct flash
    // I/O is allowed here. Serialize into an owned heap buffer and hand it to the
    // nd_store_writer task, which also creates missing parent directories from its
    // internal-RAM stack before opening the file.
    const std::string filePath = GetEntityFilePath(networkId, entityId);
    char* json_buf = (char*)heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!json_buf) {
        json_buf = (char*)malloc(len); // Fallback to internal heap
    }
    if (!json_buf) {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Malloc failed for write buffer");
        return;
    }
    memcpy(json_buf, serializedData.data(), len);

    if (!netdiscovery_submit_store_write(filePath.c_str(), json_buf, len)) {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Async write submit failed for %s", filePath.c_str());
        free(json_buf); // Ownership was not transferred
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
        ESP_LOGD(TAG, "[FileKnowledgeStore] Directory does not exist yet: %s", netDir.c_str());
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
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return true; // Directory already exists
    }
    
    // Recursively ensure parent directory exists
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos && pos > 0) {
        std::string parent = path.substr(0, pos);
        EnsureDirectoryExists(parent);
    }

    // Create directory since it's now safe from Internal RAM
    if (mkdir(path.c_str(), 0777) == 0 || errno == EEXIST) {
        return true;
    }
    
    ESP_LOGE(TAG, "[FileKnowledgeStore] Failed to create directory %s", path.c_str());
    return false;
}

} // namespace NetDiscovery
