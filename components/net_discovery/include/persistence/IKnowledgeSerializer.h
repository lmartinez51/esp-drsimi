/**
 * @file IKnowledgeSerializer.h
 * @brief Abstract Serializer Interface for KnowledgeEntity objects.
 * Decouples repositories from specific data formats (JSON, CBOR, SQLite, MessagePack).
 */

#pragma once

#include "core/KnowledgeEntity.h"

#include <string>
#include <vector>
#include <optional>

namespace NetDiscovery {
namespace Persistence {

class IKnowledgeSerializer {
public:
    virtual ~IKnowledgeSerializer() = default;

    /**
     * @brief Serializes a KnowledgeEntity C++ object into a string buffer.
     */
    virtual std::string Serialize(const KnowledgeEntity& entity) = 0;

    /**
     * @brief Deserializes a string buffer into a KnowledgeEntity C++ object.
     * Supports versioned schema interpretation.
     */
    virtual std::optional<KnowledgeEntity> Deserialize(const std::string& rawBuffer) = 0;

    /**
     * @brief Extracts schema version from a raw buffer without full deserialization.
     */
    virtual int ExtractSchemaVersion(const std::string& rawBuffer) = 0;
};

} // namespace Persistence
} // namespace NetDiscovery
