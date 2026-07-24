/**
 * @file StorageManager.h
 * @brief Exclusive ownership facade for ESP-Claw Platform Persistence & Knowledge Subsystem.
 */

#pragma once

#include "persistence/IKnowledgeRepository.h"
#include "persistence/ISettingsRepository.h"
#include "persistence/IAssetRepository.h"
#include "persistence/RepositoryContext.h"
#include "core/StorageEventBus.h"
#include "core/KnowledgeGraph.h"

#include <memory>
#include <string>

namespace NetDiscovery {
namespace Persistence {

/**
 * @brief Unified container & facade for ESP-Claw storage & graph subsystem.
 * Holds exclusive std::unique_ptr ownership of domain repositories and owns StorageEventBus & KnowledgeGraph.
 */
class StorageManager {
public:
    /**
     * @brief Construct StorageManager with explicit dependency injection.
     */
    StorageManager(
        std::unique_ptr<IKnowledgeRepository> knowledgeRepo,
        std::unique_ptr<ISettingsRepository> settingsRepo,
        std::unique_ptr<IAssetRepository> assetRepo,
        std::shared_ptr<RepositoryContext> context = nullptr,
        std::shared_ptr<StorageEventBus> eventBus = nullptr,
        std::shared_ptr<KnowledgeGraph> graph = nullptr);

    ~StorageManager() = default;

    // Factory method for default POSIX/NVS backing implementations
    static std::unique_ptr<StorageManager> CreateDefault(const std::string& basePath = "/littlefs");

    // Subsystem Accessors
    IKnowledgeRepository& Knowledge() { return *m_knowledgeRepo; }
    ISettingsRepository& Settings() { return *m_settingsRepo; }
    IAssetRepository& Assets() { return *m_assetRepo; }
    StorageEventBus& EventBus() { return *m_eventBus; }
    KnowledgeGraph& Graph() { return *m_graph; }

    const std::shared_ptr<StorageEventBus>& EventBusPtr() const { return m_eventBus; }
    const std::shared_ptr<KnowledgeGraph>& GraphPtr() const { return m_graph; }
    const std::shared_ptr<RepositoryContext>& Context() const { return m_context; }

private:
    std::unique_ptr<IKnowledgeRepository> m_knowledgeRepo;
    std::unique_ptr<ISettingsRepository> m_settingsRepo;
    std::unique_ptr<IAssetRepository> m_assetRepo;
    std::shared_ptr<RepositoryContext> m_context;
    std::shared_ptr<StorageEventBus> m_eventBus;
    std::shared_ptr<KnowledgeGraph> m_graph;
};

} // namespace Persistence
} // namespace NetDiscovery
