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

    const std::string filePath = GetEntityFilePath(networkId, entityId);
    char* json_buf = (char*)heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!json_buf) {
        json_buf = (char*)malloc(len);
    }
    if (!json_buf) {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Malloc falló para buffer de escritura");
        return;
    }
    memcpy(json_buf, serializedData.data(), len);

    if (!netdiscovery_submit_store_write(filePath.c_str(), json_buf, len)) {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Falla al enviar escritura asíncrona para %s", filePath.c_str());
        free(json_buf);
    }
}

std::string FileKnowledgeStore::LoadEntityData(const std::string& networkId, 
                                               const std::string& entityId) {
    const std::string filePath = GetEntityFilePath(networkId, entityId);
    
    struct stat st;
    if (stat(filePath.c_str(), &st) != 0 || st.st_size <= 0) {
        return "";
    }

    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) {
        return "";
    }

    size_t size = (size_t)st.st_size;
    char* safe_buf = (char*)heap_caps_malloc(size + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!safe_buf) safe_buf = (char*)malloc(size + 1);
    if (!safe_buf) {
        ESP_LOGE(TAG, "[FileKnowledgeStore] Malloc falló para buffer de lectura (%u bytes)", (unsigned)size);
        fclose(f);
        return "";
    }

    size_t read_bytes = fread(safe_buf, 1, size, f);
    safe_buf[read_bytes] = '\0';
    fclose(f);

    std::string result(safe_buf, read_bytes);
    free(safe_buf);
    
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
                std::string fileName = entry->d_name;
                if (fileName.size() > 5 && fileName.substr(fileName.size() - 5) == ".json") {
                    
                    // Abrir directamente el archivo encontrado por readdir para evitar errores de sanitización de ruta
                    std::string fullPath = netDir + "/" + fileName;
                    
                    struct stat st;
                    if (stat(fullPath.c_str(), &st) == 0 && st.st_size > 0) {
                        FILE* f = fopen(fullPath.c_str(), "rb");
                        if (f) {
                            size_t size = (size_t)st.st_size;
                            char* safe_buf = (char*)heap_caps_malloc(size + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                            if (!safe_buf) safe_buf = (char*)malloc(size + 1);
                            if (safe_buf) {
                                size_t read_bytes = fread(safe_buf, 1, size, f);
                                safe_buf[read_bytes] = '\0';
                                std::string data(safe_buf, read_bytes);
                                free(safe_buf);
                                
                                if (!data.empty()) {
                                    entities.push_back(std::move(data));
                                    ESP_LOGI(TAG, "📂 [KnowledgeStore] Entidad restaurada de Flash: %s (%u bytes)",
                                             fileName.c_str(), (unsigned)read_bytes);
                                }
                            }
                            fclose(f);
                        }
                    } else {
                        ESP_LOGW(TAG, "⚠️ [KnowledgeStore] Archivo corrupto o vacío en Flash: %s (Ignorado)", fileName.c_str());
                    }
                }
            }
        }
        closedir(dir);
        ESP_LOGI(TAG, "✅ [KnowledgeStore] Total entidades cargadas para la red '%s': %d",
                 networkId.c_str(), (int)entities.size());
    } else {
        ESP_LOGD(TAG, "[FileKnowledgeStore] El directorio aún no existe: %s", netDir.c_str());
    }

    return entities;
}

void FileKnowledgeStore::DeleteEntityData(const std::string& networkId, 
                                          const std::string& entityId) {
    const std::string filePath = GetEntityFilePath(networkId, entityId);
    netdiscovery_submit_store_delete(filePath.c_str());
}

std::string FileKnowledgeStore::GetNetworkDir(const std::string& networkId) const {
    return m_baseDir + "/" + networkId;
}

std::string FileKnowledgeStore::GetEntityFilePath(const std::string& networkId, const std::string& entityId) const {
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
        return true;
    }
    
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos && pos > 0) {
        std::string parent = path.substr(0, pos);
        EnsureDirectoryExists(parent);
    }

    if (mkdir(path.c_str(), 0777) == 0 || errno == EEXIST) {
        return true;
    }
    
    ESP_LOGE(TAG, "[FileKnowledgeStore] Falla al crear directorio %s", path.c_str());
    return false;
}

} // namespace NetDiscovery