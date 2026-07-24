/**
 * @file EntityResolutionEngine.h
 * @brief Canonical Entity Resolution Engine for ESP-Claw Platform (v5.0.0 Architecture).
 * Transforms protocol observations into canonical KnowledgeEntities using priority identity matching rules.
 */

#pragma once

#include "core/Observation.h"
#include "core/ResolutionResult.h"
#include "core/KnowledgeEntity.h"
#include "core/StorageEventBus.h"
#include "persistence/IKnowledgeRepository.h"

#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace NetDiscovery {

/**
 * @brief Canonical Entity Resolution Engine.
 * Resolves incoming protocol observations against persistent knowledge memory.
 */
class EntityResolutionEngine {
public:
    explicit EntityResolutionEngine(
        Persistence::IKnowledgeRepository& repository,
        std::shared_ptr<StorageEventBus> eventBus = nullptr);

    ~EntityResolutionEngine() = default;

    /**
     * @brief Main resolution pipeline entry point.
     * Evaluates identity priority rules, resolves/creates entity, updates repository,
     * publishes events via StorageEventBus, and returns explicit ResolutionResult.
     */
    ResolutionResult Resolve(const Observation& observation);

private:
    // Identity Priority Match Pipeline (v5 Architecture Rules)
    std::optional<KnowledgeEntity> MatchByMac(const Observation& obs, float& outConfidence);
    std::optional<KnowledgeEntity> MatchBySerial(const Observation& obs, float& outConfidence);
    std::optional<KnowledgeEntity> MatchByMatter(const Observation& obs, float& outConfidence);
    std::optional<KnowledgeEntity> MatchByBLE(const Observation& obs, float& outConfidence);
    std::optional<KnowledgeEntity> MatchByAlias(const Observation& obs, float& outConfidence);
    std::optional<KnowledgeEntity> MatchByUSN(const Observation& obs, float& outConfidence);

    // Entity Creation & Merging Helpers
    KnowledgeEntity CreateNewEntity(const Observation& obs);
    void MergeObservationIntoEntity(KnowledgeEntity& entity, const Observation& obs);
    std::string GeneratePersistentId(const Observation& obs);
    void PublishEvent(StorageEventType type, const std::string& entityId, const std::string& sourceStr);

    Persistence::IKnowledgeRepository& m_repository;
    std::shared_ptr<StorageEventBus> m_eventBus;
    mutable std::mutex m_engineMutex;
};

} // namespace NetDiscovery
