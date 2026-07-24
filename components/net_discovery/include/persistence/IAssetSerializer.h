/**
 * @file IAssetSerializer.h
 * @brief Abstract Serializer Interface for AssetMetadata.
 */

#pragma once

#include "persistence/IAssetRepository.h"
#include <string>
#include <optional>

namespace NetDiscovery {
namespace Persistence {

class IAssetSerializer {
public:
    virtual ~IAssetSerializer() = default;

    virtual std::string SerializeMetadata(const AssetMetadata& metadata) = 0;
    virtual std::optional<AssetMetadata> DeserializeMetadata(const std::string& rawBuffer) = 0;
};

} // namespace Persistence
} // namespace NetDiscovery
