#include "services/KnowledgeStore.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include "esp_log.h"

static const char* TAG = "NetDiscovery";

extern bool g_verbose;

namespace NetDiscovery {

KnowledgeStore::KnowledgeStore(std::unique_ptr<IKnowledgeStore> backend)
    : m_backend(std::move(backend))
{
}

void KnowledgeStore::Initialize() {
    if (m_backend) {
        m_backend->Initialize();
    }
}

void KnowledgeStore::ResolveKnownNetwork(const NetworkFingerprint& network) {
    m_currentNetwork = network;
    m_entities.clear();

    if (!m_backend) return;

    std::vector<std::string> rawDataList = m_backend->LoadAllEntities(network.CalculateId());
    for (const auto& raw : rawDataList) {
        KnowledgeEntity entity = DeserializeEntity(raw);
        if (!entity.persistentId.empty()) {
            m_entities[entity.persistentId] = entity;
        } else {
            ESP_LOGE(TAG, "[KnowledgeStore] Failed to deserialize entity or empty ID.");
        }
    }
}

void KnowledgeStore::UpdateFromDiscovery(const LogicalDevice& liveDevice) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::string entityId = liveDevice.id;
    if (entityId.empty()) {
        if (!liveDevice.endpoints.empty() && !liveDevice.endpoints[0].uuid.empty()) {
            entityId = liveDevice.endpoints[0].uuid;
        } else if (!liveDevice.displayName.empty()) {
            entityId = liveDevice.displayName;
        } else {
            entityId = "unknown_device_" + std::to_string(now);
        }
    }

    if (m_entities.find(entityId) == m_entities.end()) {
        // Nueva Entidad
        KnowledgeEntity newEntity;
        newEntity.persistentId = entityId;
        newEntity.lastObservedIdentity = entityId;
        newEntity.displayName = liveDevice.displayName.empty() ? entityId : liveDevice.displayName;
        newEntity.aliases.systemAliases.push_back(newEntity.displayName);
        newEntity.primaryClass = liveDevice.primaryClass;
        newEntity.roles = liveDevice.roles;
        newEntity.capabilities = liveDevice.capabilities;
        newEntity.capabilityProfiles = liveDevice.capabilityProfiles;
        newEntity.endpoints = liveDevice.endpoints;
        newEntity.firstDiscovered = now;
        newEntity.lastSeen = now;
        
        newEntity.identity.vendor = liveDevice.manufacturer;
        newEntity.identity.model = liveDevice.model;
        newEntity.identity.serialNumber = liveDevice.serialNumber;
        newEntity.normalizedServices = liveDevice.normalizedServices;
        newEntity.services = liveDevice.services;
        
        for (const auto& candidate : liveDevice.controllerCandidates) {
            newEntity.compatibleControllers.push_back(candidate);
        }
        
        AddJournalEntry(newEntity, JournalEventType::Discovered, "Newly discovered on network.");
        m_entities[entityId] = newEntity;
    } else {
        // Merge No Destructivo con Entidad Existente
        KnowledgeEntity& existing = m_entities[entityId];
        existing.lastObservedIdentity = entityId;
        existing.lastSeen = now;
        
        if (!liveDevice.displayName.empty()) {
            existing.displayName = liveDevice.displayName;
        }
        if (!liveDevice.manufacturer.empty()) existing.identity.vendor = liveDevice.manufacturer;
        if (!liveDevice.model.empty()) existing.identity.model = liveDevice.model;
        if (!liveDevice.serialNumber.empty()) existing.identity.serialNumber = liveDevice.serialNumber;
        
        if (liveDevice.primaryClass != PrimaryDeviceClass::Unknown) {
            existing.primaryClass = liveDevice.primaryClass;
        }

        if (!liveDevice.roles.empty()) existing.roles = liveDevice.roles;
        if (!liveDevice.normalizedServices.empty()) existing.normalizedServices = liveDevice.normalizedServices;
        if (!liveDevice.services.empty()) existing.services = liveDevice.services;
        
        MergeCapabilities(existing, liveDevice.capabilities);
        MergeCapabilityProfiles(existing, liveDevice.capabilityProfiles);
        MergeEndpoints(existing, liveDevice.endpoints);
        
        AddJournalEntry(existing, JournalEventType::Validated, "Validated via active discovery.");
    }

    PersistEntity(m_entities[entityId]);
}

void KnowledgeStore::UpdateCredentials(const std::string& deviceId, const std::string& key, const std::string& value) {
    for (auto& pair : m_entities) {
        if (pair.second.persistentId == deviceId || pair.second.lastObservedIdentity == deviceId) {
            pair.second.credentials[key] = value;
            PersistEntity(pair.second);
            break;
        }
    }
}

void KnowledgeStore::ArchiveEntity(const std::string& entityId) {
    if (m_entities.find(entityId) != m_entities.end()) {
        AddJournalEntry(m_entities[entityId], JournalEventType::Archived, "User archived entity.");
        PersistEntity(m_entities[entityId]);
    }
}

void KnowledgeStore::AppendCommunicationRecord(const std::string& entityId, const CommunicationRecord& record) {
    if (m_entities.find(entityId) != m_entities.end()) {
        KnowledgeEntity& entity = m_entities[entityId];
        entity.commHistory.push_back(record);
        
        JournalEventType evType = (record.status == ExecutionStatus::Success) ? JournalEventType::CommSucceeded : JournalEventType::CommFailed;
        AddJournalEntry(entity, evType, "Transport: " + record.transportName);
        
        PersistEntity(entity);
    }
}

KnowledgeConfidence KnowledgeStore::ComputeConfidence(const KnowledgeEntity& entity) const {
    KnowledgeConfidence conf;
    conf.score = 50;
    
    if (!entity.commHistory.empty()) {
        const auto& lastComm = entity.commHistory.back();
        if (lastComm.status == ExecutionStatus::Success) {
            conf.score += 30;
            conf.computedState = KnowledgeState::Validated;
        } else {
            conf.score -= 20;
            conf.computedState = KnowledgeState::Stale;
        }
    }
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    long long ageMs = now - entity.lastSeen;
    
    if (ageMs > 86400000) {
        conf.score -= 40;
        conf.computedState = KnowledgeState::Stale;
    }
    
    if (conf.score < 0) conf.score = 0;
    if (conf.score > 100) conf.score = 100;
    
    return conf;
}

std::vector<KnowledgeEntity>& KnowledgeStore::GetLoadedEntities() {
    static std::vector<KnowledgeEntity> temp;
    temp.clear();
    for (auto& kv : m_entities) {
        temp.push_back(kv.second);
    }
    return temp;
}

void KnowledgeStore::MergeEndpoints(KnowledgeEntity& existing, const std::vector<ProtocolEndpoint>& liveEndpoints) {
    for (const auto& liveEp : liveEndpoints) {
        bool found = false;
        for (auto& existEp : existing.endpoints) {
            if (existEp.ip == liveEp.ip) {
                existEp = liveEp; 
                found = true;
                break;
            }
        }
        if (!found) {
            existing.endpoints.push_back(liveEp);
        }
    }
}

void KnowledgeStore::MergeCapabilities(KnowledgeEntity& existing, const std::vector<Capability>& liveCaps) {
    for (const auto& liveCap : liveCaps) {
        existing.capabilities.AddCapability(liveCap);
    }
}

void KnowledgeStore::MergeCapabilityProfiles(KnowledgeEntity& existing, const std::vector<CapabilityProfile>& liveProfiles) {
    for (const auto& liveProfile : liveProfiles) {
        bool found = false;
        for (auto& existProfile : existing.capabilityProfiles) {
            if (existProfile.capability == liveProfile.capability) {
                for (const auto& liveAct : liveProfile.supportedActions) {
                    bool actFound = false;
                    for (auto& existAct : existProfile.supportedActions) {
                        if (existAct.actionId == liveAct.actionId) {
                            existAct = liveAct;
                            actFound = true;
                            break;
                        }
                    }
                    if (!actFound) {
                        existProfile.supportedActions.push_back(liveAct);
                    }
                }
                found = true;
                break;
            }
        }
        if (!found) {
            existing.capabilityProfiles.push_back(liveProfile);
        }
    }
}

void KnowledgeStore::AddJournalEntry(KnowledgeEntity& entity, JournalEventType type, const std::string& description) {
    JournalEntry entry;
    entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    entry.type = type;
    entry.description = description;
    entity.journal.push_back(entry);
}

// -------------------------------------------------------------------------
// Serialization (Corregida y Blindada)
// -------------------------------------------------------------------------

std::string KnowledgeStore::SerializeEntity(const KnowledgeEntity& entity) const {
    std::ostringstream oss;
    oss << "SCHEMA_VERSION=" << entity.schemaVersion << "\n";
    oss << "PERSISTENT_ID=" << entity.persistentId << "\n";
    oss << "DISPLAY_NAME=" << entity.displayName << "\n";
    oss << "PRIMARY_CLASS=" << static_cast<int>(entity.primaryClass) << "\n";
    oss << "VENDOR=" << entity.identity.vendor << "\n";
    oss << "MODEL=" << entity.identity.model << "\n";
    oss << "SERIAL=" << entity.identity.serialNumber << "\n";
    
    if (!entity.compatibleControllers.empty()) {
        oss << "CONTROLLERS=";
        for (size_t i = 0; i < entity.compatibleControllers.size(); ++i) {
            oss << entity.compatibleControllers[i].name << ":" 
                << entity.compatibleControllers[i].confidence << ":"
                << (entity.compatibleControllers[i].isRejected ? "1" : "0")
                << (i + 1 == entity.compatibleControllers.size() ? "" : ",");
        }
        oss << "\n";
    }

    auto caps = entity.capabilities.GetCapabilities();
    if (!caps.empty()) {
        oss << "CAPABILITIES=";
        for (size_t i = 0; i < caps.size(); ++i) {
            oss << caps[i].id << (i + 1 == caps.size() ? "" : ",");
        }
        oss << "\n";
    }

    for (const auto& profile : entity.capabilityProfiles) {
        oss << "CPROFILE=" << profile.capability.id << "|" << profile.globalConstraints;
        for (const auto& act : profile.supportedActions) {
            oss << "|" << static_cast<int>(act.actionId) << ":" 
                << static_cast<int>(act.supportState) << ":" 
                << act.constraints << ":" 
                << static_cast<int>(act.reason);
        }
        oss << "\n";
    }

    if (!entity.roles.empty()) {
        oss << "ROLES=";
        for (size_t i = 0; i < entity.roles.size(); ++i) {
            oss << static_cast<int>(entity.roles[i]) << (i + 1 == entity.roles.size() ? "" : ",");
        }
        oss << "\n";
    }

    for (const auto& ep : entity.endpoints) {
        oss << "ENDPOINT=" << ep.ip << "|" << ep.serverHeader << "|" << ep.uuid;
        if (ep.evidence.upnp.has_value()) {
            oss << "|" << ep.evidence.upnp->locationUrl << "|" << ep.evidence.upnp->applicationUrl;
        }
        oss << "\n";
    }

    return oss.str();
}

KnowledgeEntity KnowledgeStore::DeserializeEntity(const std::string& data) const {
    KnowledgeEntity entity;
    std::istringstream iss(data);
    std::string line;

    while (std::getline(iss, line)) {
        // Sanitizar caracteres de control al final de la línea (\r, \n)
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
            line.pop_back();
        }

        if (line.empty()) continue;

        auto eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);

            if (key == "PERSISTENT_ID") {
                entity.persistentId = val;
            } else if (key == "DISPLAY_NAME") {
                entity.displayName = val;
            } else if (key == "PRIMARY_CLASS") {
                if (!val.empty()) {
                    entity.primaryClass = static_cast<PrimaryDeviceClass>(std::strtol(val.c_str(), nullptr, 10));
                }
            } else if (key == "VENDOR") {
                entity.identity.vendor = val;
            } else if (key == "MODEL") {
                entity.identity.model = val;
            } else if (key == "SERIAL") {
                entity.identity.serialNumber = val;
            } else if (key == "FIRST_DISCOVERED") {
                entity.firstDiscovered = std::strtoll(val.c_str(), nullptr, 10);
            } else if (key == "LAST_SEEN") {
                entity.lastSeen = std::strtoll(val.c_str(), nullptr, 10);
            } else if (key == "CONTROLLERS") {
                std::istringstream css(val);
                std::string c;
                while (std::getline(css, c, ',')) {
                    if (c.empty()) continue;
                    std::istringstream partss(c);
                    std::string token;
                    std::vector<std::string> subparts;
                    while (std::getline(partss, token, ':')) {
                        subparts.push_back(token);
                    }
                    ControllerCandidate cand;
                    if (subparts.size() >= 1) cand.name = subparts[0];
                    if (subparts.size() >= 2) cand.confidence = std::strtol(subparts[1].c_str(), nullptr, 10);
                    if (subparts.size() >= 3) cand.isRejected = (subparts[2] == "1");
                    entity.compatibleControllers.push_back(cand);
                }
            } else if (key == "CAPABILITIES") {
                std::istringstream css(val);
                std::string c;
                while (std::getline(css, c, ',')) {
                    if (!c.empty()) {
                        char* endptr = nullptr;
                        long numericCap = std::strtol(c.c_str(), &endptr, 10);
                        if (endptr != c.c_str() && *endptr == '\0') {
                            entity.capabilities.AddCapability(static_cast<Capability>(numericCap));
                        } else {
                            entity.capabilities.AddCapability(Capability(c));
                        }
                    }
                }
            } else if (key == "ENDPOINT") {
                std::istringstream ess(val);
                std::string token;
                std::vector<std::string> parts;
                while (std::getline(ess, token, '|')) {
                    parts.push_back(token);
                }
                if (parts.size() >= 3) {
                    ProtocolEndpoint ep;
                    ep.ip = parts[0];
                    ep.serverHeader = parts[1];
                    ep.uuid = parts[2];
                    if (parts.size() >= 5) {
                        UPnPEvidence upnp;
                        upnp.locationUrl = parts[3];
                        upnp.applicationUrl = parts[4];
                        ep.evidence.upnp = std::move(upnp);
                    }
                    entity.endpoints.push_back(ep);

                    // FALLBACK DE ID: Si no venía PERSISTENT_ID= explícito, usar el UUID del endpoint
                    if (entity.persistentId.empty() && !ep.uuid.empty()) {
                        entity.persistentId = ep.uuid;
                    }
                }
            }
        }
    }

    // FALLBACK SECUNDARIO: Si el nombre de despliegue existe pero sigue sin ID, asignarle el nombre
    if (entity.persistentId.empty() && !entity.displayName.empty()) {
        entity.persistentId = entity.displayName;
    }

    return entity;
}

void KnowledgeStore::PersistEntity(const KnowledgeEntity& entity) {
    if (m_backend && !m_currentNetwork.CalculateId().empty()) {
        m_backend->SaveEntityData(m_currentNetwork.CalculateId(), entity.persistentId, SerializeEntity(entity));
    }
}

} // namespace NetDiscovery