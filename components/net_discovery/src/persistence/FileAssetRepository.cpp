/**
 * @file FileAssetRepository.cpp
 * @brief File-backed implementation of IAssetRepository.
 */

#include "persistence/FileAssetRepository.h"
#include "persistence/AtomicFileWriter.h"
#include "persistence/JsonAssetSerializer.h"

#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "esp_log.h"

static const char* TAG = "FileAssetRepo";

namespace NetDiscovery {
namespace Persistence {

FileAssetRepository::FileAssetRepository(
    std::shared_ptr<RepositoryContext> context,
    std::unique_ptr<IAssetSerializer> serializer)
    : m_context(context ? context : std::make_shared<RepositoryContext>()),
      m_serializer(serializer ? std::move(serializer) : std::make_unique<JsonAssetSerializer>())
{
    m_resourcesDir = m_context->GetResourcesPath();
    EnsureDirectoryExists();
}

void FileAssetRepository::EnsureDirectoryExists() {
    struct stat st;
    if (stat(m_context->GetBasePath().c_str(), &st) != 0) {
        mkdir(m_context->GetBasePath().c_str(), 0755);
    }
    if (stat(m_resourcesDir.c_str(), &st) != 0) {
        mkdir(m_resourcesDir.c_str(), 0755);
    }
}

std::string FileAssetRepository::GetMetaPath(const std::string& assetName) const {
    return m_resourcesDir + "/" + assetName + ".meta";
}

std::string FileAssetRepository::GetDataPath(const std::string& assetName) const {
    return m_resourcesDir + "/" + assetName + ".bin";
}

bool FileAssetRepository::HasAsset(const std::string& assetName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    struct stat st;
    return (stat(GetMetaPath(assetName).c_str(), &st) == 0);
}

std::optional<AssetMetadata> FileAssetRepository::GetAssetMetadata(const std::string& assetName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ifstream f(GetMetaPath(assetName));
    if (!f.is_open()) return std::nullopt;

    std::stringstream buffer;
    buffer << f.rdbuf();
    return m_serializer->DeserializeMetadata(buffer.str());
}

bool FileAssetRepository::SaveAsset(const std::string& assetName, const std::vector<uint8_t>& data, const AssetMetadata& metadata) {
    if (assetName.empty()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string metaStr = m_serializer->SerializeMetadata(metadata);
    if (metaStr.empty()) return false;

    bool okData = AtomicFileWriter::WriteBinaryAtomic(GetDataPath(assetName), data);
    bool okMeta = AtomicFileWriter::WriteStringAtomic(GetMetaPath(assetName), metaStr);

    if (okData && okMeta) {
        ESP_LOGI(TAG, "Saved asset %s (%zu bytes) atomically", assetName.c_str(), data.size());
        return true;
    }
    return false;
}

std::optional<std::vector<uint8_t>> FileAssetRepository::LoadAsset(const std::string& assetName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ifstream f(GetDataPath(assetName), std::ios::binary);
    if (!f.is_open()) return std::nullopt;

    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

bool FileAssetRepository::DeleteAsset(const std::string& assetName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    remove(GetMetaPath(assetName).c_str());
    remove(GetDataPath(assetName).c_str());
    return true;
}

std::vector<std::string> FileAssetRepository::ListAssets(const std::string& category) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> res;

    DIR* dir = opendir(m_resourcesDir.c_str());
    if (!dir) return res;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 5 && filename.substr(filename.length() - 5) == ".meta") {
            std::string assetName = filename.substr(0, filename.length() - 5);
            if (category.empty()) {
                res.push_back(assetName);
            } else {
                std::ifstream f(m_resourcesDir + "/" + filename);
                if (f.is_open()) {
                    std::stringstream buffer;
                    buffer << f.rdbuf();
                    auto metaOpt = m_serializer->DeserializeMetadata(buffer.str());
                    if (metaOpt.has_value() && metaOpt->resourceType == category) {
                        res.push_back(assetName);
                    }
                }
            }
        }
    }
    closedir(dir);
    return res;
}

} // namespace Persistence
} // namespace NetDiscovery
