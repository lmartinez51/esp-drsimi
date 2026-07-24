/**
 * @file ResolutionResult.h
 * @brief Explicit resolution result structure for EntityResolutionEngine.
 */

#pragma once

#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Action determined by the resolution pipeline.
 */
enum class ResolutionAction {
    ExistingEntity,   // Observation matched an existing canonical entity; entity was merged
    NewEntity,        // Observation resulted in a new canonical entity creation
    MergeEntities,    // Observation resolved multiple candidate records into one merged entity
    Conflict          // Contradictory identities detected (requires manual or AI resolution)
};

/**
 * @brief Converts ResolutionAction to a printable string.
 */
inline std::string ToString(ResolutionAction action) {
    switch (action) {
        case ResolutionAction::ExistingEntity: return "ExistingEntity";
        case ResolutionAction::NewEntity:      return "NewEntity";
        case ResolutionAction::MergeEntities:   return "MergeEntities";
        case ResolutionAction::Conflict:        return "Conflict";
        default:                                return "Unknown";
    }
}

/**
 * @brief Explicit outcome of an observation resolution operation.
 */
struct ResolutionResult {
    ResolutionAction action{ResolutionAction::NewEntity};
    std::string entityId;
    float confidence{0.0f};
    std::vector<std::string> matchedRules;
};

} // namespace NetDiscovery
