/**
 * @file JsonAssetSerializer.cpp
 * @brief Versioned JSON implementation of IAssetSerializer using cJSON.
 */

#include "persistence/JsonAssetSerializer.h"

#include <cstdlib>
#include "cJSON.h"

namespace NetDiscovery {
namespace Persistence {

std::string JsonAssetSerializer::SerializeMetadata(const AssetMetadata& metadata) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "asset_name", metadata.assetName.c_str());
    cJSON_AddStringToObject(root, "resource_type", metadata.resourceType.c_str());
    cJSON_AddStringToObject(root, "resource_version", metadata.resourceVersion.c_str());
    cJSON_AddStringToObject(root, "minimum_supported_firmware", metadata.minimumSupportedFirmware.c_str());
    cJSON_AddStringToObject(root, "asset_hash", metadata.assetHash.c_str());
    cJSON_AddBoolToObject(root, "migration_required", metadata.migrationRequired);

    char* formatted = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!formatted) return "";

    std::string result(formatted);
    free(formatted);
    return result;
}

std::optional<AssetMetadata> JsonAssetSerializer::DeserializeMetadata(const std::string& rawBuffer) {
    if (rawBuffer.empty()) return std::nullopt;

    cJSON* root = cJSON_Parse(rawBuffer.c_str());
    if (!root || !cJSON_IsObject(root)) {
        if (root) cJSON_Delete(root);
        return std::nullopt;
    }

    AssetMetadata metadata;

    cJSON* name = cJSON_GetObjectItem(root, "asset_name");
    if (name && cJSON_IsString(name)) metadata.assetName = name->valuestring;

    cJSON* type = cJSON_GetObjectItem(root, "resource_type");
    if (type && cJSON_IsString(type)) metadata.resourceType = type->valuestring;

    cJSON* ver = cJSON_GetObjectItem(root, "resource_version");
    if (ver && cJSON_IsString(ver)) metadata.resourceVersion = ver->valuestring;

    cJSON* minFw = cJSON_GetObjectItem(root, "minimum_supported_firmware");
    if (minFw && cJSON_IsString(minFw)) metadata.minimumSupportedFirmware = minFw->valuestring;

    cJSON* hash = cJSON_GetObjectItem(root, "asset_hash");
    if (hash && cJSON_IsString(hash)) metadata.assetHash = hash->valuestring;

    cJSON* mig = cJSON_GetObjectItem(root, "migration_required");
    if (mig && cJSON_IsBool(mig)) metadata.migrationRequired = cJSON_IsTrue(mig);

    cJSON_Delete(root);
    return metadata;
}

} // namespace Persistence
} // namespace NetDiscovery
