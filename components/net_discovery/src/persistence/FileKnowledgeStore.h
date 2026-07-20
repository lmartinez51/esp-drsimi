/**
 * @file FileKnowledgeStore.h
 * @brief Windows filesystem-based implementation of IKnowledgeStore.
 */

#pragma once

#include "persistence/IKnowledgeStore.h"
#include <string>

namespace NetDiscovery {

class FileKnowledgeStore : public IKnowledgeStore {
public:
    explicit FileKnowledgeStore(const std::string& baseDir = "data/knowledge");
    ~FileKnowledgeStore() override = default;

    void Initialize() override;

    void SaveEntityData(const std::string& networkId, 
                        const std::string& entityId, 
                        const std::string& serializedData) override;

    std::string LoadEntityData(const std::string& networkId, 
                               const std::string& entityId) override;

    std::vector<std::string> LoadAllEntities(const std::string& networkId) override;

    void DeleteEntityData(const std::string& networkId, 
                          const std::string& entityId) override;

private:
    std::string m_baseDir;

    std::string GetNetworkDir(const std::string& networkId) const;
    std::string GetEntityFilePath(const std::string& networkId, const std::string& entityId) const;
    bool EnsureDirectoryExists(const std::string& path) const;
};

} // namespace NetDiscovery
