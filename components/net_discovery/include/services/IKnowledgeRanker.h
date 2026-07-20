/**
 * @file IKnowledgeRanker.h
 * @brief Interface for prioritizing knowledge entities during startup or resolution.
 */

#pragma once

#include "core/KnowledgeEntity.h"
#include <vector>
#include <memory>

namespace NetDiscovery {

/**
 * @brief Sorts and filters entities based on historical reliability, aliases, or context.
 */
class IKnowledgeRanker {
public:
    virtual ~IKnowledgeRanker() = default;

    /**
     * @brief Takes a list of entities and returns a sorted/ranked list.
     * Higher priority entities (e.g., frequently used) should appear first.
     */
    virtual std::vector<KnowledgeEntity*> Rank(std::vector<KnowledgeEntity>& entities) = 0;
};

} // namespace NetDiscovery
