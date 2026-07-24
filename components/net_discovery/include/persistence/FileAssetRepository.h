/**
 * @file FileAssetRepository.h
 * @brief File-backed implementation of IAssetRepository using RepositoryContext and IAssetSerializer.
 */

#pragma once

#include "persistence/IAssetRepository.h"
#include "persistence/RepositoryContext.h"
#include "persistence/IAssetSerializer.h"

#include <memory>
#include <mutex>
#include <map>

namespace NetDiscovery {
namespace Persistence {

class FileAssetRepository : public IAssetRepository {
public:
    explicit FileAssetRepository(
        std::shared_ptr<RepositoryContext> context = nullptr,
        std::unique_ptr<IAssetSerializer> serializer = nullptr);
    ~FileAssetRepository() override = default;

    bool HasAsset(const std::string& assetName) override;
    std::optional<AssetMetadata> GetAssetMetadata(const std::string& assetName) override;
    bool SaveAsset(const std::string& assetName, const std::vector<uint8_t>& data, const AssetMetadata& metadata) override;
    std::optional<std::vector<uint8_t>> LoadAsset(const std::string& assetName) override;
    bool DeleteAsset(const std::string& assetName) override;
    std::vector<std::string> ListAssets(const std::string& category = "") override;

private:
    void EnsureDirectoryExists();
    std::string GetMetaPath(const std::string& assetName) const;
    std::string GetDataPath(const std::string& assetName) const;

    std::shared_ptr<RepositoryContext> m_context;
    std::unique_ptr<IAssetSerializer> m_serializer;
    std::string m_resourcesDir;
    mutable std::mutex m_mutex;
};

} // namespace Persistence
} // namespace NetDiscovery
