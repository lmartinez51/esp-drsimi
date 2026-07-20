/**
 * @file KnowledgeEntity.h
 * @brief Represents the long-lived, serialized snapshot of a domain entity.
 */

#pragma once

#include "KnowledgeModels.h"
#include "DeviceClass.h"
#include "Capability.h"
#include "CapabilityProfile.h"
#include "ProtocolEndpoint.h"
#include "ControllerCandidate.h"

#include <string>
#include <vector>
#include <map>

namespace NetDiscovery {

/**
 * @brief The persistent knowledge record of an entity (Device, Room, etc).
 * This model contains ONLY long-lived data and should be kept free of 
 * runtime logic and dynamic states like confidence.
 */
struct KnowledgeEntity {
    int schemaVersion = 1;
    EntityType type = EntityType::Device;
    
    // ----------------------------------------------------------------
    // Identity
    // ----------------------------------------------------------------
    std::string persistentId;          // Stable, permanent identifier across sessions
    std::string lastObservedIdentity;  // The transient runtime ID from the most recent discovery (LogicalDevice::id)
    
    std::string displayName;           // Primary display name
    EntityAliases aliases;             // System and user aliases for Semantic routing
    
    // ----------------------------------------------------------------
    // Payload (Device-Specific properties, empty for Rooms/Groups)
    // ----------------------------------------------------------------
    PrimaryDeviceClass primaryClass{PrimaryDeviceClass::Unknown};
    std::vector<DeviceRole> roles;
    std::vector<Capability> capabilities;
    std::vector<CapabilityProfile> capabilityProfiles;
    std::vector<ControllerCandidate> compatibleControllers; // Resolved dynamically via ControllerRegistry, persisted with confidence
    std::vector<ProtocolEndpoint> endpoints;
    std::map<std::string, std::string> credentials; // Persisted authentication data
    
    // ----------------------------------------------------------------
    // Communication & History
    // ----------------------------------------------------------------
    std::vector<CommunicationRecord> commHistory;
    std::vector<JournalEntry> journal; // Chronological history of lifecycle events
    
    // ----------------------------------------------------------------
    // Lifespan
    // ----------------------------------------------------------------
    long long firstDiscovered{0};
    long long lastSeen{0};
};

} // namespace NetDiscovery
