/**
 * @file StorageEventType.h
 * @brief Domain event types supported by ESP-Claw StorageEventBus (Architecture v5.0.0 Phase 10.1).
 */

#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief Domain event types emitted across the ESP-Claw platform.
 */
enum class StorageEventType {
    EntityCreated,
    EntityUpdated,
    EntityMerged,
    EntityArchived,
    EntityDeleted,
    EndpointChanged,
    CapabilityAdded,
    CapabilityRemoved,
    CapabilityUpdated,
    RelationshipCreated,
    RelationshipRemoved,
    AssetProvisioned,
    SettingsChanged,
    GraphUpdated,
    // Phase 8 Action Binding Engine Domain Events
    BindingRegistered,
    BindingRemoved,
    BindingUpdated,
    AdapterRegistered,
    AdapterUpdated,
    AdapterHealthChanged,
    PreferredBindingChanged,
    // Phase 8.5 & 8.6 Execution Planning Domain Events
    ExecutionPlanCreated,
    ExecutionPlanValidated,
    ExecutionPlanRejected,
    ExecutionPlanCancelled,
    ExecutionPlanExpired,
    // Phase 9 Runtime Execution Engine Domain Events
    ExecutionStarted,
    ExecutionPaused,
    ExecutionResumed,
    ExecutionCancelled,
    ExecutionCompleted,
    ExecutionFailed,
    ExecutionStepStarted,
    ExecutionStepCompleted,
    ExecutionStepFailed,
    ExecutionWaiting,
    ExecutionTimeout,
    RollbackStarted,
    RollbackCompleted,
    RollbackFailed,
    // Phase 10 Protocol Adapter Framework Lifecycle Events
    AdapterRemoved,
    AdapterInitialized,
    AdapterShutdown,
    AdapterAvailable,
    AdapterUnavailable,
    AdapterHealthRefreshed,
    // Phase 10.1 Adapter Resolution Events
    AdapterResolved,
    AdapterResolutionFailed,
    CapabilityMismatch
};

/**
 * @brief Converts StorageEventType to string.
 */
inline std::string ToString(StorageEventType type) {
    switch (type) {
        case StorageEventType::EntityCreated:           return "EntityCreated";
        case StorageEventType::EntityUpdated:           return "EntityUpdated";
        case StorageEventType::EntityMerged:            return "EntityMerged";
        case StorageEventType::EntityArchived:          return "EntityArchived";
        case StorageEventType::EntityDeleted:           return "EntityDeleted";
        case StorageEventType::EndpointChanged:         return "EndpointChanged";
        case StorageEventType::CapabilityAdded:         return "CapabilityAdded";
        case StorageEventType::CapabilityRemoved:       return "CapabilityRemoved";
        case StorageEventType::CapabilityUpdated:       return "CapabilityUpdated";
        case StorageEventType::RelationshipCreated:     return "RelationshipCreated";
        case StorageEventType::RelationshipRemoved:     return "RelationshipRemoved";
        case StorageEventType::AssetProvisioned:        return "AssetProvisioned";
        case StorageEventType::SettingsChanged:         return "SettingsChanged";
        case StorageEventType::GraphUpdated:            return "GraphUpdated";
        case StorageEventType::BindingRegistered:       return "BindingRegistered";
        case StorageEventType::BindingRemoved:          return "BindingRemoved";
        case StorageEventType::BindingUpdated:          return "BindingUpdated";
        case StorageEventType::AdapterRegistered:       return "AdapterRegistered";
        case StorageEventType::AdapterUpdated:          return "AdapterUpdated";
        case StorageEventType::AdapterHealthChanged:    return "AdapterHealthChanged";
        case StorageEventType::PreferredBindingChanged: return "PreferredBindingChanged";
        case StorageEventType::ExecutionPlanCreated:    return "ExecutionPlanCreated";
        case StorageEventType::ExecutionPlanValidated:  return "ExecutionPlanValidated";
        case StorageEventType::ExecutionPlanRejected:   return "ExecutionPlanRejected";
        case StorageEventType::ExecutionPlanCancelled:  return "ExecutionPlanCancelled";
        case StorageEventType::ExecutionPlanExpired:    return "ExecutionPlanExpired";
        case StorageEventType::ExecutionStarted:        return "ExecutionStarted";
        case StorageEventType::ExecutionPaused:         return "ExecutionPaused";
        case StorageEventType::ExecutionResumed:        return "ExecutionResumed";
        case StorageEventType::ExecutionCancelled:      return "ExecutionCancelled";
        case StorageEventType::ExecutionCompleted:      return "ExecutionCompleted";
        case StorageEventType::ExecutionFailed:         return "ExecutionFailed";
        case StorageEventType::ExecutionStepStarted:    return "ExecutionStepStarted";
        case StorageEventType::ExecutionStepCompleted:  return "ExecutionStepCompleted";
        case StorageEventType::ExecutionStepFailed:     return "ExecutionStepFailed";
        case StorageEventType::ExecutionWaiting:        return "ExecutionWaiting";
        case StorageEventType::ExecutionTimeout:        return "ExecutionTimeout";
        case StorageEventType::RollbackStarted:         return "RollbackStarted";
        case StorageEventType::RollbackCompleted:       return "RollbackCompleted";
        case StorageEventType::RollbackFailed:          return "RollbackFailed";
        case StorageEventType::AdapterRemoved:          return "AdapterRemoved";
        case StorageEventType::AdapterInitialized:      return "AdapterInitialized";
        case StorageEventType::AdapterShutdown:         return "AdapterShutdown";
        case StorageEventType::AdapterAvailable:        return "AdapterAvailable";
        case StorageEventType::AdapterUnavailable:      return "AdapterUnavailable";
        case StorageEventType::AdapterHealthRefreshed:  return "AdapterHealthRefreshed";
        case StorageEventType::AdapterResolved:         return "AdapterResolved";
        case StorageEventType::AdapterResolutionFailed: return "AdapterResolutionFailed";
        case StorageEventType::CapabilityMismatch:     return "CapabilityMismatch";
        default:                                        return "Unknown";
    }
}

} // namespace NetDiscovery
