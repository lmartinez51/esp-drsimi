/**
 * @file StorageManager.cpp
 * @brief Implementation of StorageManager container & factory using KnowledgeGraph.
 */

#include "persistence/StorageManager.h"
#include "persistence/InMemoryKnowledgeRepository.h"
#include "persistence/NvsSettingsRepository.h"
#include "persistence/FileAssetRepository.h"

namespace NetDiscovery {
namespace Persistence {

StorageManager::StorageManager(
    std::unique_ptr<IKnowledgeRepository> knowledgeRepo,
    std::unique_ptr<ISettingsRepository> settingsRepo,
    std::unique_ptr<IAssetRepository> assetRepo,
    std::shared_ptr<RepositoryContext> context,
    std::shared_ptr<StorageEventBus> eventBus,
    std::shared_ptr<KnowledgeGraph> graph)
    : m_knowledgeRepo(std::move(knowledgeRepo)),
      m_settingsRepo(std::move(settingsRepo)),
      m_assetRepo(std::move(assetRepo)),
      m_context(context ? context : std::make_shared<RepositoryContext>()),
      m_eventBus(eventBus ? eventBus : std::make_shared<StorageEventBus>()),
      m_graph(graph ? graph : std::make_shared<KnowledgeGraph>(m_eventBus))
{
}

std::unique_ptr<StorageManager> StorageManager::CreateDefault(const std::string& basePath) {
    auto context = std::make_shared<RepositoryContext>(basePath);
    auto eventBus = std::make_shared<StorageEventBus>();
    auto graph = std::make_shared<KnowledgeGraph>(eventBus);

    auto knowledgeRepo = std::make_unique<InMemoryKnowledgeRepository>(context, nullptr, eventBus);
    auto settingsRepo = std::make_unique<NvsSettingsRepository>(context);
    auto assetRepo = std::make_unique<FileAssetRepository>(context);

    // Rebuild KnowledgeGraph adjacency from loaded entities on startup (O(N+E))
    graph->RebuildFromEntities(knowledgeRepo->GetAllEntities());

    return std::make_unique<StorageManager>(
        std::move(knowledgeRepo),
        std::move(settingsRepo),
        std::move(assetRepo),
        context,
        eventBus,
        graph
    );
}

} // namespace Persistence
} // namespace NetDiscovery
