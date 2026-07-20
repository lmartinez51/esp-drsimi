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

    // Map LogicalDevice -> KnowledgeEntity
    // If it exists in memory, merge it. If not, create it.
    
    // Simple lookup by identity (LogicalDevice::id is transient, but for this basic implementation we assume it's stable enough to map to persistentId if no other matching logic exists. In a full implementation, we'd do a deep evidence match.)
    std::string entityId = liveDevice.id;

    if (m_entities.find(entityId) == m_entities.end()) {
        // New Entity
        KnowledgeEntity newEntity;
        newEntity.persistentId = entityId;
        newEntity.lastObservedIdentity = liveDevice.id;
        newEntity.displayName = liveDevice.displayName;
        newEntity.aliases.systemAliases.push_back(liveDevice.displayName);
        newEntity.primaryClass = liveDevice.primaryClass;
        newEntity.roles = liveDevice.roles;
        newEntity.capabilities = liveDevice.capabilities;
        newEntity.capabilityProfiles = liveDevice.capabilityProfiles;
        newEntity.endpoints = liveDevice.endpoints;
        newEntity.firstDiscovered = now;
        newEntity.lastSeen = now;
        
        for (const auto& candidate : liveDevice.controllerCandidates) {
            newEntity.compatibleControllers.push_back(candidate);
        }
        
        AddJournalEntry(newEntity, JournalEventType::Discovered, "Newly discovered on network.");
        m_entities[entityId] = newEntity;
    } else {
        // Merge Existing Entity
        KnowledgeEntity& existing = m_entities[entityId];
        existing.lastObservedIdentity = liveDevice.id;
        existing.lastSeen = now;
        
        // Live overrides persisted (optimization)
        if (!liveDevice.displayName.empty()) {
            existing.displayName = liveDevice.displayName;
        }
        existing.primaryClass = liveDevice.primaryClass;
        existing.roles = liveDevice.roles; // Overwrite
        
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
        // We do NOT delete it from backend. It just stays archived.
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
    conf.score = 50; // Base score
    
    // Check if there's any communication history
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
    
    // Check aging
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    long long ageMs = now - entity.lastSeen;
    
    if (ageMs > 86400000) { // Older than 1 day
        conf.score -= 40;
        conf.computedState = KnowledgeState::Stale;
    }
    
    if (conf.score < 0) conf.score = 0;
    if (conf.score > 100) conf.score = 100;
    
    return conf;
}

std::vector<KnowledgeEntity>& KnowledgeStore::GetLoadedEntities() {
    // This isn't efficient, but sufficient for Phase 5.5 placeholder
    static std::vector<KnowledgeEntity> temp;
    temp.clear();
    for (auto& kv : m_entities) {
        temp.push_back(kv.second);
    }
    return temp; // Warning: returns static vector reference. Refactor for thread safety later.
}

// -------------------------------------------------------------------------
// Internal Merge Logic
// -------------------------------------------------------------------------

void KnowledgeStore::MergeEndpoints(KnowledgeEntity& existing, const std::vector<ProtocolEndpoint>& liveEndpoints) {
    for (const auto& liveEp : liveEndpoints) {
        bool found = false;
        for (auto& existEp : existing.endpoints) {
            // Very simplified endpoint matching
            if (existEp.ip == liveEp.ip) {
                // Update it
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
        bool found = false;
        for (const auto& existCap : existing.capabilities) {
            if (existCap == liveCap) {
                found = true;
                break;
            }
        }
        if (!found) {
            existing.capabilities.push_back(liveCap);
        }
    }
}

void KnowledgeStore::MergeCapabilityProfiles(KnowledgeEntity& existing, const std::vector<CapabilityProfile>& liveProfiles) {
    for (const auto& liveProfile : liveProfiles) {
        bool found = false;
        for (auto& existProfile : existing.capabilityProfiles) {
            if (existProfile.capability == liveProfile.capability) {
                // Merge actions
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
// Serialization (Lightweight Key-Value approach to avoid external libs)
// -------------------------------------------------------------------------

std::string KnowledgeStore::SerializeEntity(const KnowledgeEntity& entity) const {
    std::ostringstream oss;
    oss << "SCHEMA_VERSION=" << entity.schemaVersion << "\n";
    oss << "PERSISTENT_ID=" << entity.persistentId << "\n";
    oss << "DISPLAY_NAME=" << entity.displayName << "\n";
    oss << "FIRST_DISCOVERED=" << entity.firstDiscovered << "\n";
    oss << "LAST_SEEN=" << entity.lastSeen << "\n";
    
    // Arrays as comma-separated or pipe-separated lists
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

    if (!entity.capabilities.empty()) {
        oss << "CAPABILITIES=";
        for (size_t i = 0; i < entity.capabilities.size(); ++i) {
            oss << static_cast<int>(entity.capabilities[i]) << (i + 1 == entity.capabilities.size() ? "" : ",");
        }
        oss << "\n";
    }

    for (const auto& profile : entity.capabilityProfiles) {
        oss << "CPROFILE=" << static_cast<int>(profile.capability) << "|" << profile.globalConstraints;
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

    // Endpoints (we save multiple ENDPOINT lines: IP|LocationUrl|ApplicationUrl)
    for (const auto& ep : entity.endpoints) {
        oss << "ENDPOINT=" << ep.ip << "|" << ep.serverHeader << "|" << ep.uuid;
        if (ep.evidence.upnp.has_value()) {
            if (g_verbose && !ep.evidence.upnp->applicationUrl.empty()) {
                std::cout << "[Metadata] KnowledgeStore serializing Application-URL: " << ep.evidence.upnp->applicationUrl << "\n";
            }
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
        auto eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "PERSISTENT_ID") entity.persistentId = val;
            else if (key == "DISPLAY_NAME") entity.displayName = val;
            else if (key == "FIRST_DISCOVERED") entity.firstDiscovered = std::strtoll(val.c_str(), nullptr, 10);
            else if (key.find("CREDENTIAL:") == 0) {
                std::string credKey = key.substr(11);
                entity.credentials[credKey] = val;
            }
            else if (key == "LAST_SEEN") entity.lastSeen = std::strtoll(val.c_str(), nullptr, 10);
            else if (key == "CONTROLLERS") {
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
            }
            else if (key == "CAPABILITIES") {
                std::istringstream css(val);
                std::string c;
                while (std::getline(css, c, ',')) {
                    if (!c.empty()) entity.capabilities.push_back(static_cast<Capability>(std::strtol(c.c_str(), nullptr, 10)));
                }
            }
            else if (key == "CPROFILE") {
                std::istringstream css(val);
                std::string token;
                std::vector<std::string> parts;
                while (std::getline(css, token, '|')) {
                    parts.push_back(token);
                }
                if (parts.size() >= 2) {
                    CapabilityProfile profile;
                    profile.capability = static_cast<Capability>(std::strtol(parts[0].c_str(), nullptr, 10));
                    profile.globalConstraints = std::strtoul(parts[1].c_str(), nullptr, 10);
                    for (size_t i = 2; i < parts.size(); ++i) {
                        std::istringstream ass(parts[i]);
                        std::string aToken;
                        std::vector<std::string> aParts;
                        while (std::getline(ass, aToken, ':')) {
                            aParts.push_back(aToken);
                        }
                        if (aParts.size() == 4) {
                            SupportedActionProfile sap;
                            sap.actionId = static_cast<ActionId>(std::strtol(aParts[0].c_str(), nullptr, 10));
                            sap.supportState = static_cast<SupportState>(std::strtol(aParts[1].c_str(), nullptr, 10));
                            sap.constraints = std::strtoul(aParts[2].c_str(), nullptr, 10);
                            sap.reason = static_cast<ConstraintReason>(std::strtol(aParts[3].c_str(), nullptr, 10));
                            profile.supportedActions.push_back(sap);
                        }
                    }
                    entity.capabilityProfiles.push_back(profile);
                }
            }
            else if (key == "ROLES") {
                std::istringstream css(val);
                std::string c;
                while (std::getline(css, c, ',')) {
                    if (!c.empty()) entity.roles.push_back(static_cast<DeviceRole>(std::strtol(c.c_str(), nullptr, 10)));
                }
            }
            else if (key == "ENDPOINT") {
                // Parse: IP|ServerHeader|UUID|LocationUrl|ApplicationUrl
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
                    if (parts.size() >= 5) { // Has UPnP evidence
                        UPnPEvidence upnp;
                        upnp.locationUrl = parts[3];
                        upnp.applicationUrl = parts[4];
                        if (g_verbose && !upnp.applicationUrl.empty()) {
                            std::cout << "[Metadata] KnowledgeStore deserialized Application-URL: " << upnp.applicationUrl << "\n";
                        }
                        ep.evidence.upnp = std::move(upnp);
                    }
                    entity.endpoints.push_back(ep);
                }
            }
        }
    }
    return entity;
}

void KnowledgeStore::PersistEntity(const KnowledgeEntity& entity) {
    if (m_backend && !m_currentNetwork.CalculateId().empty()) {
        m_backend->SaveEntityData(m_currentNetwork.CalculateId(), entity.persistentId, SerializeEntity(entity));
    }
}

} // namespace NetDiscovery
