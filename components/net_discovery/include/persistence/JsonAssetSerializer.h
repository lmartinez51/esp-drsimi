/**
 * @file JsonAssetSerializer.h
 * @brief Versioned JSON implementation of IAssetSerializer using cJSON.
 */

#pragma once

#include "persistence/IAssetSerializer.h"

namespace NetDiscovery {
namespace Persistence {

class JsonAssetSerializer : public IAssetSerializer {
public:
    JsonAssetSerializer() = default;
    ~JsonAssetSerializer() override = default;

    std::string SerializeMetadata(const AssetMetadata& metadata) override;
    std::optional<AssetMetadata> DeserializeMetadata(const std::string& rawBuffer) override;
};

} // namespace Persistence
} // namespace NetDiscovery
