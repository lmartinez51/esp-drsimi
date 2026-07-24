/**
 * @file JsonKnowledgeSerializer.h
 * @brief Versioned JSON implementation of IKnowledgeSerializer using cJSON.
 */

#pragma once

#include "persistence/IKnowledgeSerializer.h"

namespace NetDiscovery {
namespace Persistence {

class JsonKnowledgeSerializer : public IKnowledgeSerializer {
public:
    JsonKnowledgeSerializer() = default;
    ~JsonKnowledgeSerializer() override = default;

    std::string Serialize(const KnowledgeEntity& entity) override;
    std::optional<KnowledgeEntity> Deserialize(const std::string& rawBuffer) override;
    int ExtractSchemaVersion(const std::string& rawBuffer) override;
};

} // namespace Persistence
} // namespace NetDiscovery
