/**
 * @file KnowledgeSynchronizer.h
 * @brief Decouples ExecutionEngine from KnowledgeStore.
 */

#pragma once

#include "core/ExecutionResult.h"
#include "services/KnowledgeStore.h"
#include <string>

namespace NetDiscovery {

/**
 * @brief Translates execution feedback into knowledge communication records.
 */
class KnowledgeSynchronizer {
public:
    explicit KnowledgeSynchronizer(KnowledgeStore& store);

    /**
     * @brief Called after the ExecutionEngine finishes a command.
     */
    void OnExecutionCompleted(const std::string& entityId, 
                              const std::string& transportName, 
                              const ExecutionResult& result);

private:
    KnowledgeStore& m_store;
};

} // namespace NetDiscovery
