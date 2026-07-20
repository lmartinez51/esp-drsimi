/**
 * @file IKnowledgeStore.h
 * @brief Pure mechanical persistence mechanism.
 */

#pragma once

#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Strictly mechanical interface for saving and loading serialized entities.
 * 
 * This interface contains NO business logic, NO merging, and NO knowledge of 
 * the schema. It only understands network keys, entity keys, and bytes.
 */
class IKnowledgeStore {
public:
    virtual ~IKnowledgeStore() = default;

    /**
     * @brief Prepares the storage backend (e.g., creating directories or opening NVS).
     */
    virtual void Initialize() = 0;

    /**
     * @brief Persists an entity's serialized data for a specific network.
     */
    virtual void SaveEntityData(const std::string& networkId, 
                                const std::string& entityId, 
                                const std::string& serializedData) = 0;

    /**
     * @brief Loads an entity's serialized data.
     * @return The serialized data, or an empty string if not found.
     */
    virtual std::string LoadEntityData(const std::string& networkId, 
                                       const std::string& entityId) = 0;

    /**
     * @brief Loads all serialized entity records for a given network.
     */
    virtual std::vector<std::string> LoadAllEntities(const std::string& networkId) = 0;

    /**
     * @brief Physically deletes an entity's data from storage.
     */
    virtual void DeleteEntityData(const std::string& networkId, 
                                  const std::string& entityId) = 0;
};

} // namespace NetDiscovery
