#include "services/KnowledgeStore.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <cstring>
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

    std::string networkId = network.CalculateId();
    std::vector<std::string> rawDataList = m_backend->LoadAllEntities(networkId);
    for (const auto& raw : rawDataList) {
        KnowledgeEntity entity = DeserializeEntity(raw);
        if (!entity.persistentId.empty()) {
            m_entities[entity.persistentId] = entity;
        } else {
            ESP_LOGE(TAG, "[KnowledgeStore] Failed to deserialize entity or empty ID.");
        }
    }

    // Boot-time self-healing: merge legacy duplicate entities that share the same IP/MAC.
    if (m_entities.size() > 1) {
        ConsolidateDuplicates(networkId);
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

    // Tombstone check: reject any re-discovery of a forgotten entity (by ID or IP)
    if (m_tombstones.count(entityId) > 0 || m_tombstones.count(liveDevice.id) > 0) {
        ESP_LOGI(TAG, "\xf0\x9f\x9b\xa1\xef\xb8\x8f [KnowledgeStore] Entity '%s' is tombstoned for active session. Discovery update ignored.", entityId.c_str());
        return;
    }
    if (!liveDevice.endpoints.empty() && !liveDevice.endpoints[0].ip.empty()) {
        if (m_tombstones.count(liveDevice.endpoints[0].ip) > 0) {
            ESP_LOGI(TAG, "\xf0\x9f\x9b\xa1\xef\xb8\x8f [KnowledgeStore] IP '%s' is tombstoned. Discovery update ignored.",
                     liveDevice.endpoints[0].ip.c_str());
            return;
        }
    }

    // IP-based deduplication: if a new UUID arrives for an IP we already track,
    // redirect to the canonical entity instead of creating a duplicate entry.
    if (m_entities.find(entityId) == m_entities.end()) {
        if (!liveDevice.endpoints.empty() && !liveDevice.endpoints[0].ip.empty()) {
            const std::string& liveIp = liveDevice.endpoints[0].ip;
            for (const auto& kv : m_entities) {
                for (const auto& ep : kv.second.endpoints) {
                    if (!ep.ip.empty() && ep.ip == liveIp) {
                        ESP_LOGI(TAG, "[KnowledgeStore] IP-dedup: redirecting new ID '%s' -> canonical '%s' (shared IP: %s)",
                                 entityId.c_str(), kv.first.c_str(), liveIp.c_str());
                        entityId = kv.first; // merge into canonical
                        goto ip_dedup_done;
                    }
                }
            }
        }
    }
    ip_dedup_done:

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

std::vector<KnowledgeEntity> KnowledgeStore::GetLoadedEntities() const {
    std::vector<KnowledgeEntity> result;
    result.reserve(m_entities.size());
    for (const auto& kv : m_entities) {
        result.push_back(kv.second);
    }
    return result;
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

bool KnowledgeStore::RemoveEntity(const std::string& entityId) {
    m_tombstones.insert(entityId);

    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) {
        if (!it->second.persistentId.empty()) m_tombstones.insert(it->second.persistentId);
        if (!it->second.displayName.empty()) m_tombstones.insert(it->second.displayName);
        for (const auto& ep : it->second.endpoints) {
            if (!ep.uuid.empty()) m_tombstones.insert(ep.uuid);
            if (!ep.ip.empty()) m_tombstones.insert(ep.ip);
        }

        std::string pId = it->second.persistentId.empty() ? entityId : it->second.persistentId;
        m_entities.erase(it);

        if (m_backend && !m_currentNetwork.CalculateId().empty()) {
            m_backend->DeleteEntityData(m_currentNetwork.CalculateId(), pId);
        }
        ESP_LOGI(TAG, "\xf0\x9f\x9b\xa1\xef\xb8\x8f [KnowledgeStore] Entity '%s' removed from RAM/Flash & registered in tombstones.", entityId.c_str());
        return true;
    }
    ESP_LOGW(TAG, "[KnowledgeStore] Entity '%s' not found in m_entities during RemoveEntity, registered tombstone.", entityId.c_str());
    return false;
}

void KnowledgeStore::ConsolidateDuplicates(const std::string& networkId) {
    // Maps primary IP -> canonical persistentId (first entity found that owns that IP)
    std::map<std::string, std::string> ipToCanonical;
    // Maps MAC -> canonical persistentId
    std::map<std::string, std::string> macToCanonical;
    // Duplicate IDs to erase from m_entities after iteration
    std::vector<std::string> toRemove;

    for (auto& kv : m_entities) {
        const std::string& candidateId = kv.first;
        KnowledgeEntity& candidate = kv.second;
        bool isDuplicate = false;
        std::string canonicalId;

        // --- Check by primary IP ---
        for (const auto& ep : candidate.endpoints) {
            if (ep.ip.empty()) continue;
            auto ipIt = ipToCanonical.find(ep.ip);
            if (ipIt != ipToCanonical.end()) {
                canonicalId = ipIt->second;
                isDuplicate = true;
                break;
            }
        }

        // --- Check by hardware MAC (if not already flagged as duplicate) ---
        if (!isDuplicate && !candidate.identity.macAddress.empty()) {
            auto macIt = macToCanonical.find(candidate.identity.macAddress);
            if (macIt != macToCanonical.end()) {
                canonicalId = macIt->second;
                isDuplicate = true;
            }
        }

        if (isDuplicate) {
            // Merge this duplicate into the canonical entity
            KnowledgeEntity& canonical = m_entities[canonicalId];
            MergeEndpoints(canonical, candidate.endpoints);
            MergeCapabilities(canonical, candidate.capabilities.GetCapabilities());
            MergeCapabilityProfiles(canonical, candidate.capabilityProfiles);
            // Carry over controllers not already present
            for (const auto& ctrl : candidate.compatibleControllers) {
                bool found = false;
                for (const auto& existing : canonical.compatibleControllers) {
                    if (existing.name == ctrl.name) { found = true; break; }
                }
                if (!found) canonical.compatibleControllers.push_back(ctrl);
            }
            // Prefer the richer displayName
            if (canonical.displayName.empty() && !candidate.displayName.empty()) {
                canonical.displayName = candidate.displayName;
            }

            ESP_LOGW(TAG, "[KnowledgeStore] \xf0\x9f\x94\xa7 Auto-consolidation: merged duplicate '%s' into canonical '%s'. Purging from LittleFS.",
                     candidateId.c_str(), canonicalId.c_str());

            // Permanently delete the redundant file from LittleFS
            if (m_backend && !networkId.empty()) {
                m_backend->DeleteEntityData(networkId, candidateId);
            }
            toRemove.push_back(candidateId);
        } else {
            // Register all IPs and MAC of this canonical entity
            for (const auto& ep : candidate.endpoints) {
                if (!ep.ip.empty()) ipToCanonical.emplace(ep.ip, candidateId);
            }
            if (!candidate.identity.macAddress.empty()) {
                macToCanonical.emplace(candidate.identity.macAddress, candidateId);
            }
        }
    }

    // Remove duplicates from in-memory map
    for (const auto& id : toRemove) {
        m_entities.erase(id);
    }

    if (!toRemove.empty()) {
        ESP_LOGI(TAG, "[KnowledgeStore] Auto-consolidation complete: removed %d duplicate entities. Re-persisting canonicals.",
                 (int)toRemove.size());
        // Re-persist all canonicals to ensure merged data is reflected on Flash
        for (auto& kv : m_entities) {
            PersistEntity(kv.second);
        }
    }
}

int KnowledgeStore::FindEntityForAdmin(const char* targetLower, std::string& outId, std::string& outDisplay) const {
    // All matching done in-place over m_entities — no heap allocation on caller side.
    // Three-pass priority: 1=exact IP, 2=exact UUID/persistentId, 3=substring on name/vendor/model.
    const KnowledgeEntity* singleMatch = nullptr;
    int matchCount = 0;

    // Pass 1: Exact IP match
    for (const auto& kv : m_entities) {
        for (const auto& ep : kv.second.endpoints) {
            if (!ep.ip.empty() && strcmp(ep.ip.c_str(), targetLower) == 0) {
                singleMatch = &kv.second;
                matchCount++;
                break;
            }
        }
    }

    // Pass 2: Exact UUID / persistentId match (only if Pass 1 found nothing)
    if (matchCount == 0) {
        for (const auto& kv : m_entities) {
            if (strcmp(kv.second.persistentId.c_str(), targetLower) == 0) {
                singleMatch = &kv.second;
                matchCount++;
            } else {
                for (const auto& ep : kv.second.endpoints) {
                    if (!ep.uuid.empty() && strcmp(ep.uuid.c_str(), targetLower) == 0) {
                        singleMatch = &kv.second;
                        matchCount++;
                        break;
                    }
                }
            }
        }
    }

    // Pass 3: Bidirectional case-insensitive substring on displayName, vendor, model (only if still nothing)
    if (matchCount == 0 && targetLower[0] != '\0') {
        char tmp[64];
        for (const auto& kv : m_entities) {
            bool found = false;

            // Check displayName
            strlcpy(tmp, kv.second.displayName.c_str(), sizeof(tmp));
            for (char* p = tmp; *p; ++p) *p = (char)std::tolower((unsigned char)*p);
            if (tmp[0]) {
                if (strstr(tmp, targetLower) || (strlen(tmp) >= 3 && strstr(targetLower, tmp))) {
                    found = true;
                }
            }

            // Check vendor
            if (!found) {
                strlcpy(tmp, kv.second.identity.vendor.c_str(), sizeof(tmp));
                for (char* p = tmp; *p; ++p) *p = (char)std::tolower((unsigned char)*p);
                if (tmp[0]) {
                    if (strstr(tmp, targetLower) || (strlen(tmp) >= 3 && strstr(targetLower, tmp))) {
                        found = true;
                    }
                }
            }

            // Check model
            if (!found) {
                strlcpy(tmp, kv.second.identity.model.c_str(), sizeof(tmp));
                for (char* p = tmp; *p; ++p) *p = (char)std::tolower((unsigned char)*p);
                if (tmp[0]) {
                    if (strstr(tmp, targetLower) || (strlen(tmp) >= 3 && strstr(targetLower, tmp))) {
                        found = true;
                    }
                }
            }

            if (found) {
                singleMatch = &kv.second;
                matchCount++;
            }
        }
    }

    if (matchCount == 1 && singleMatch) {
        outId      = singleMatch->persistentId;
        outDisplay = singleMatch->displayName;
    }
    return matchCount;
}

} // namespace NetDiscovery