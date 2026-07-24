/**
 * @file KnowledgeEntity.h
 * @brief Represents the canonical Knowledge Entity model in ESP-Claw (v5.0.0 Architecture).
 */

#pragma once

#include "KnowledgeModels.h"
#include "DeviceClass.h"
#include "Capability.h"
#include "CapabilityProfile.h"
#include "ProtocolEndpoint.h"
#include "ControllerCandidate.h"
#include "NormalizedService.h"
#include "ServiceDescriptor.h"

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace NetDiscovery {

/**
 * @brief Relationship type for entity graph edges.
 */
struct EntityRelationship {
    std::string type; // LOCATED_IN, USES, PAIRED_WITH, CONTROLLED_BY, PARENT_OF, CHILD_OF, GROUP_MEMBER
    std::string targetId;
};

/**
 * @brief 1. Permanent Identity Layer (Immutable hardware facts)
 */
struct EntityIdentity {
    std::string macAddress;
    std::string serialNumber;
    std::string vendor;
    std::string model;
    PrimaryDeviceClass primaryClass{PrimaryDeviceClass::Unknown};
    CapabilitySet capabilities;
    std::vector<CapabilityType> legacyCapabilities;
    std::vector<ControllerCandidate> supportedControllers;
};

/**
 * @brief 2. Ephemeral Runtime State (Transient channel state)
 */
struct EntityRuntimeState {
    bool isOnline{false};
    int activeEndpointIndex{0};
    int rssi{0};
    int latencyMs{0};
    int batteryPct{100};
    std::vector<ProtocolEndpoint> endpoints;
};

/**
 * @brief 3. AI Annotation Layer (Cognitive memory)
 */
struct EntityAIAnnotations {
    std::string summary;
    std::string userNotes;
    std::string confidenceReason;
    std::vector<std::string> semanticTags;
    std::string embeddingReference;
};

/**
 * @brief 4. Lifecycle Governance Metadata
 */
struct EntityLifecycle {
    std::string state{"ACTIVE"};           // ACTIVE, OFFLINE, STALE, ARCHIVED, SOFT_DELETED
    std::string retentionPolicy{"AUTO"};   // AUTO, PINNED, TEMPORARY
    bool userDeleted{false};
    int confidenceScore{100};
    int revision{1};
    int64_t createdAt{0};
    int64_t lastSeen{0};
    int64_t lastSuccess{0};
    int64_t lastModified{0};
    int64_t archivedAt{0};
    uint32_t timesSeen{0};
    uint32_t timesUsed{0};
    uint32_t timesFailed{0};
};

/**
 * @brief The canonical Knowledge Entity record of ESP-Claw.
 */
struct KnowledgeEntity {
    int schemaVersion = 5;
    EntityType type = EntityType::Device;
    
    // Primary Identity
    std::string persistentId;          // Stable, permanent identifier across sessions
    std::string lastObservedIdentity;  // Transient runtime discovery ID
    std::string displayName;           // Primary display name
    EntityAliases aliases;             // System and user aliases

    // Modular Sub-structures (v5.0.0 Architecture)
    EntityIdentity identity;
    EntityRuntimeState runtimeState;
    EntityAIAnnotations aiAnnotations;
    EntityLifecycle lifecycle;
    std::vector<EntityRelationship> relationships;

    // Backwards Compatibility Fields
    PrimaryDeviceClass primaryClass{PrimaryDeviceClass::Unknown};
    std::vector<DeviceRole> roles;
    CapabilitySet capabilities;
    std::vector<CapabilityType> legacyCapabilities;
    std::vector<CapabilityProfile> capabilityProfiles;
    std::vector<ControllerCandidate> compatibleControllers;
    std::vector<NormalizedService> normalizedServices;
    std::vector<ServiceDescriptor> services;
    std::vector<ProtocolEndpoint> endpoints;
    std::map<std::string, std::string> credentials;
    
    std::vector<CommunicationRecord> commHistory;
    std::vector<JournalEntry> journal;
    
    long long firstDiscovered{0};
    long long lastSeen{0};
};

} // namespace NetDiscovery
