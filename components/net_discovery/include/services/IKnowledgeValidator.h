/**
 * @file IKnowledgeValidator.h
 * @brief Interface for validating persisted entities against the live network.
 */

#pragma once

#include "core/KnowledgeEntity.h"
#include <string>

namespace NetDiscovery {

/**
 * @brief Rich result of a validation attempt.
 */
struct ValidationResult {
    bool success{false};
    int latencyMs{0};
    std::string protocolUsed;
    std::string failureReason;
    std::string retryRecommendation;
    int confidenceContribution{0}; // +/- adjustment to confidence score
};

/**
 * @brief Abstracts the mechanism used to check if an entity is still online/valid.
 */
class IKnowledgeValidator {
public:
    virtual ~IKnowledgeValidator() = default;

    /**
     * @brief Attempts to validate the entity.
     */
    virtual ValidationResult Validate(const KnowledgeEntity& entity) = 0;
};

} // namespace NetDiscovery
