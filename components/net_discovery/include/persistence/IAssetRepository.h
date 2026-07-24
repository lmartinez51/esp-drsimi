/**
 * @file IAssetRepository.h
 * @brief Platform System Resources Repository Interface for generic firmware assets under /resources/.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace NetDiscovery {
namespace Persistence {

struct AssetMetadata {
    std::string assetName;
    std::string resourceType;
    std::string resourceVersion;
    std::string minimumSupportedFirmware;
    std::string assetHash;
    bool migrationRequired{false};
};

class IAssetRepository {
public:
    virtual ~IAssetRepository() = default;

    virtual bool HasAsset(const std::string& assetName) = 0;
    virtual std::optional<AssetMetadata> GetAssetMetadata(const std::string& assetName) = 0;
    virtual bool SaveAsset(const std::string& assetName, const std::vector<uint8_t>& data, const AssetMetadata& metadata) = 0;
    virtual std::optional<std::vector<uint8_t>> LoadAsset(const std::string& assetName) = 0;
    virtual bool DeleteAsset(const std::string& assetName) = 0;
    virtual std::vector<std::string> ListAssets(const std::string& category = "") = 0;
};

} // namespace Persistence
} // namespace NetDiscovery
