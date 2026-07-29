/**
 * @file EntityResolutionEngine.cpp
 * @brief Implementation of Canonical Entity Resolution Engine with StorageEventBus integration.
 */

#include "services/EntityResolutionEngine.h"

#include <chrono>
#include <algorithm>

#include "esp_log.h"

static const char* TAG = "EntityResolutionEngine";

namespace NetDiscovery {

EntityResolutionEngine::EntityResolutionEngine(
    Persistence::IKnowledgeRepository& repository,
    std::shared_ptr<StorageEventBus> eventBus)
    : m_repository(repository),
      m_eventBus(eventBus)
{
}

ResolutionResult EntityResolutionEngine::Resolve(const Observation& observation) {
    std::lock_guard<std::mutex> lock(m_engineMutex);

    std::vector<std::string> matchedRules;
    float highestConfidence = 0.0f;
    std::optional<KnowledgeEntity> matchedEntity = std::nullopt;

    // ----------------------------------------------------------------
    // Priority 1: MAC Address (Confidence: 100.0)
    // ----------------------------------------------------------------
    if (!observation.macAddress.empty()) {
        float conf = 0.0f;
        auto res = MatchByMac(observation, conf);
        if (res.has_value()) {
            matchedEntity = res;
            highestConfidence = conf;
            matchedRules.push_back("MAC_ADDRESS_MATCH (100%)");
        }
    }

    // Priority 1.5: IP Address Match (Consolidar servicios en la misma IP física)
    if (!matchedEntity.has_value() && !observation.endpoints.empty()) {
        const std::string& obsIp = observation.endpoints[0].ip;
        if (!obsIp.empty() && obsIp != "0.0.0.0") {
            auto entities = m_repository.GetAllEntities();
            for (const auto& entity : entities) {
                for (const auto& ep : entity.endpoints) {
                    if (ep.ip == obsIp) {
                        matchedEntity = entity;
                        highestConfidence = 85.0f;
                        matchedRules.push_back("IP_ADDRESS_MATCH (85%)");
                        break;
                    }
                }
                if (matchedEntity.has_value()) break;
            }
        }
    }

    // ----------------------------------------------------------------
    // Priority 2: Serial Number (Confidence: 95.0)
    // ----------------------------------------------------------------
    if (!matchedEntity.has_value() && !observation.serialNumber.empty()) {
        float conf = 0.0f;
        auto res = MatchBySerial(observation, conf);
        if (res.has_value()) {
            matchedEntity = res;
            highestConfidence = conf;
            matchedRules.push_back("SERIAL_NUMBER_MATCH (95%)");
        }
    }

    // ----------------------------------------------------------------
    // Priority 3: Matter Node ID (Confidence: 95.0)
    // ----------------------------------------------------------------
    if (!matchedEntity.has_value() && !observation.matterNodeId.empty()) {
        float conf = 0.0f;
        auto res = MatchByMatter(observation, conf);
        if (res.has_value()) {
            matchedEntity = res;
            highestConfidence = conf;
            matchedRules.push_back("MATTER_NODE_ID_MATCH (95%)");
        }
    }

    // ----------------------------------------------------------------
    // Priority 4: BLE Identity (Confidence: 90.0)
    // ----------------------------------------------------------------
    if (!matchedEntity.has_value() && !observation.bleIdentity.empty()) {
        float conf = 0.0f;
        auto res = MatchByBLE(observation, conf);
        if (res.has_value()) {
            matchedEntity = res;
            highestConfidence = conf;
            matchedRules.push_back("BLE_IDENTITY_MATCH (90%)");
        }
    }

    // ----------------------------------------------------------------
    // Priority 5: User/System Alias (Confidence: 60.0)
    // ----------------------------------------------------------------
    if (!matchedEntity.has_value() && !observation.userAlias.empty()) {
        float conf = 0.0f;
        auto res = MatchByAlias(observation, conf);
        if (res.has_value()) {
            matchedEntity = res;
            highestConfidence = conf;
            matchedRules.push_back("ALIAS_MATCH (60%)");
        }
    }

    // ----------------------------------------------------------------
    // Priority 6: UPnP USN / UUID (Confidence: 50.0)
    // ----------------------------------------------------------------
    if (!matchedEntity.has_value() && !observation.usn.empty()) {
        float conf = 0.0f;
        auto res = MatchByUSN(observation, conf);
        if (res.has_value()) {
            matchedEntity = res;
            highestConfidence = conf;
            matchedRules.push_back("UPNP_USN_MATCH (50%)");
        }
    }

    ResolutionResult result;

    if (matchedEntity.has_value()) {
        KnowledgeEntity entity = matchedEntity.value();
        MergeObservationIntoEntity(entity, observation);
        m_repository.SaveEntity(entity);

        result.action = ResolutionAction::ExistingEntity;
        result.entityId = entity.persistentId;
        result.confidence = highestConfidence;
        result.matchedRules = matchedRules;

        PublishEvent(StorageEventType::EntityMerged, entity.persistentId, ToString(observation.source));
        PublishEvent(StorageEventType::EntityUpdated, entity.persistentId, ToString(observation.source));

        ESP_LOGI(TAG, "Resolved observation to existing entity %s (confidence: %.1f%%)",
                 entity.persistentId.c_str(), highestConfidence);
    } else {
        KnowledgeEntity newEntity = CreateNewEntity(observation);
        m_repository.SaveEntity(newEntity);

        result.action = ResolutionAction::NewEntity;
        result.entityId = newEntity.persistentId;
        result.confidence = 100.0f;
        result.matchedRules.push_back("NEW_CANONICAL_ENTITY_CREATED");

        PublishEvent(StorageEventType::EntityCreated, newEntity.persistentId, ToString(observation.source));

        ESP_LOGI(TAG, "Created new canonical entity %s", newEntity.persistentId.c_str());
    }

    return result;
}

void EntityResolutionEngine::PublishEvent(StorageEventType type, const std::string& entityId, const std::string& sourceStr) {
    if (!m_eventBus) return;

    using namespace std::chrono;
    uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    StorageEvent event;
    event.type = type;
    event.entityId = entityId;
    event.timestamp = now;
    event.metadata["source"] = sourceStr;

    m_eventBus->Publish(event);
}

std::optional<KnowledgeEntity> EntityResolutionEngine::MatchByMac(const Observation& obs, float& outConfidence) {
    if (obs.macAddress.empty()) return std::nullopt;
    auto entityOpt = m_repository.FindByMac(obs.macAddress);
    if (entityOpt.has_value()) {
        outConfidence = 100.0f;
        return entityOpt;
    }
    return std::nullopt;
}

std::optional<KnowledgeEntity> EntityResolutionEngine::MatchBySerial(const Observation& obs, float& outConfidence) {
    if (obs.serialNumber.empty()) return std::nullopt;
    auto entities = m_repository.GetAllEntities();
    for (const auto& entity : entities) {
        if (!entity.identity.serialNumber.empty() && entity.identity.serialNumber == obs.serialNumber) {
            outConfidence = 95.0f;
            return entity;
        }
    }
    return std::nullopt;
}

std::optional<KnowledgeEntity> EntityResolutionEngine::MatchByMatter(const Observation& obs, float& outConfidence) {
    if (obs.matterNodeId.empty()) return std::nullopt;
    auto entities = m_repository.GetAllEntities();
    for (const auto& entity : entities) {
        auto it = entity.credentials.find("matter_node_id");
        if (it != entity.credentials.end() && it->second == obs.matterNodeId) {
            outConfidence = 95.0f;
            return entity;
        }
    }
    return std::nullopt;
}

std::optional<KnowledgeEntity> EntityResolutionEngine::MatchByBLE(const Observation& obs, float& outConfidence) {
    if (obs.bleIdentity.empty()) return std::nullopt;
    auto entities = m_repository.GetAllEntities();
    for (const auto& entity : entities) {
        if (entity.identity.macAddress == obs.bleIdentity) {
            outConfidence = 90.0f;
            return entity;
        }
        auto it = entity.credentials.find("ble_identity");
        if (it != entity.credentials.end() && it->second == obs.bleIdentity) {
            outConfidence = 90.0f;
            return entity;
        }
    }
    return std::nullopt;
}

std::optional<KnowledgeEntity> EntityResolutionEngine::MatchByAlias(const Observation& obs, float& outConfidence) {
    if (obs.userAlias.empty()) return std::nullopt;
    auto entities = m_repository.GetAllEntities();
    for (const auto& entity : entities) {
        if (entity.displayName == obs.userAlias) {
            outConfidence = 60.0f;
            return entity;
        }
        for (const auto& alias : entity.aliases.userAliases) {
            if (alias == obs.userAlias) {
                outConfidence = 60.0f;
                return entity;
            }
        }
    }
    return std::nullopt;
}

std::optional<KnowledgeEntity> EntityResolutionEngine::MatchByUSN(const Observation& obs, float& outConfidence) {
    if (obs.usn.empty()) return std::nullopt;
    auto entities = m_repository.GetAllEntities();
    for (const auto& entity : entities) {
        if (entity.lastObservedIdentity == obs.usn) {
            outConfidence = 50.0f;
            return entity;
        }
        auto it = entity.credentials.find("usn");
        if (it != entity.credentials.end() && it->second == obs.usn) {
            outConfidence = 50.0f;
            return entity;
        }
    }
    return std::nullopt;
}

std::string EntityResolutionEngine::GeneratePersistentId(const Observation& obs) {
    if (!obs.macAddress.empty()) {
        std::string cleanMac = obs.macAddress;
        for (char& c : cleanMac) if (c == ':') c = '-';
        return "device_" + cleanMac;
    }
    if (!obs.matterNodeId.empty()) {
        return "matter_node_" + obs.matterNodeId;
    }
    if (!obs.serialNumber.empty()) {
        return "sn_" + obs.serialNumber;
    }
    if (!obs.usn.empty()) {
        std::string cleanUsn = obs.usn;
        for (char& c : cleanUsn) if (c == ':' || c == '/') c = '_';
        return "upnp_" + cleanUsn;
    }

    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    return "entity_" + std::to_string(now);
}

KnowledgeEntity EntityResolutionEngine::CreateNewEntity(const Observation& obs) {
    KnowledgeEntity entity;
    entity.schemaVersion = 5;
    entity.type = EntityType::Device;
    entity.persistentId = GeneratePersistentId(obs);
    entity.lastObservedIdentity = obs.usn.empty() ? entity.persistentId : obs.usn;

    // Display Name
    if (!obs.userAlias.empty()) {
        entity.displayName = obs.userAlias;
    } else if (!obs.hostname.empty()) {
        entity.displayName = obs.hostname;
    } else if (!obs.vendor.empty() && !obs.model.empty()) {
        entity.displayName = obs.vendor + " " + obs.model;
    } else {
        entity.displayName = entity.persistentId;
    }

    // Identity Layer (Immutable hardware facts)
    entity.identity.macAddress = obs.macAddress;
    entity.identity.serialNumber = obs.serialNumber;
    entity.identity.vendor = obs.vendor;
    entity.identity.model = obs.model;

    // Backwards Compatibility Fields
    entity.capabilities = obs.capabilities;
    entity.compatibleControllers = obs.controllers;
    entity.endpoints = obs.endpoints;

    if (!obs.usn.empty()) entity.credentials["usn"] = obs.usn;
    if (!obs.matterNodeId.empty()) entity.credentials["matter_node_id"] = obs.matterNodeId;
    if (!obs.bleIdentity.empty()) entity.credentials["ble_identity"] = obs.bleIdentity;

    // Runtime State
    entity.runtimeState.isOnline = true;
    entity.runtimeState.endpoints = obs.endpoints;

    // Lifecycle Metadata
    using namespace std::chrono;
    int64_t now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    entity.lifecycle.state = "ACTIVE";
    entity.lifecycle.retentionPolicy = "AUTO";
    entity.lifecycle.userDeleted = false;
    entity.lifecycle.confidenceScore = 100;
    entity.lifecycle.revision = 1;
    entity.lifecycle.createdAt = now;
    entity.lifecycle.lastSeen = now;
    entity.lifecycle.lastSuccess = now;
    entity.lifecycle.timesSeen = 1;

    return entity;
}

void EntityResolutionEngine::MergeObservationIntoEntity(KnowledgeEntity& entity, const Observation& obs) {
    // 1. Update Ephemeral Runtime State
    entity.runtimeState.isOnline = true;
    entity.lifecycle.revision++;

    if (!obs.usn.empty()) entity.lastObservedIdentity = obs.usn;

    // 2. Merge Capabilities (Add non-duplicate capabilities)
    for (const auto& newCap : obs.capabilities) {
        entity.capabilities.AddCapability(newCap);
        entity.identity.capabilities.AddCapability(newCap);
    }

    // 3. Merge Compatible Controllers
    for (const auto& newCtrl : obs.controllers) {
        auto it = std::find_if(entity.compatibleControllers.begin(), entity.compatibleControllers.end(),
                               [&](const ControllerCandidate& c) { return c.name == newCtrl.name; });
        if (it == entity.compatibleControllers.end()) {
            entity.compatibleControllers.push_back(newCtrl);
        }
    }

    // 4. Merge Protocol Endpoints
    for (const auto& newEp : obs.endpoints) {
        auto it = std::find_if(entity.endpoints.begin(), entity.endpoints.end(),
                               [&](const ProtocolEndpoint& ep) { return ep.ip == newEp.ip || ep.uuid == newEp.uuid; });
        if (it == entity.endpoints.end()) {
            entity.endpoints.push_back(newEp);
        } else {
            if (!newEp.ip.empty()) it->ip = newEp.ip;
            if (!newEp.serverHeader.empty()) it->serverHeader = newEp.serverHeader;
        }
    }

    // 5. Fill Missing Identity Layer Fields (Only if missing)
    if (entity.identity.macAddress.empty() && !obs.macAddress.empty()) {
        entity.identity.macAddress = obs.macAddress;
    }
    if (entity.identity.serialNumber.empty() && !obs.serialNumber.empty()) {
        entity.identity.serialNumber = obs.serialNumber;
    }
    if (entity.identity.vendor.empty() && !obs.vendor.empty()) {
        entity.identity.vendor = obs.vendor;
    }
    if (entity.identity.model.empty() && !obs.model.empty()) {
        entity.identity.model = obs.model;
    }

    if (!obs.usn.empty()) entity.credentials["usn"] = obs.usn;
    if (!obs.matterNodeId.empty()) entity.credentials["matter_node_id"] = obs.matterNodeId;
    if (!obs.bleIdentity.empty()) entity.credentials["ble_identity"] = obs.bleIdentity;
}

} // namespace NetDiscovery
