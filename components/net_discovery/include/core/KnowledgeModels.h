/**
 * @file KnowledgeModels.h
 * @brief Core types and structures for the Knowledge Layer.
 */

#pragma once

#include "ExecutionResult.h" // For ExecutionStatus
#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Categorization of a KnowledgeEntity.
 */
enum class EntityType {
    Device,
    Room,
    Group,
    Scene
};

/**
 * @brief The dynamically computed confidence state of an entity.
 * Note: This state is NOT persisted; it is computed at runtime based on evidence.
 */
enum class KnowledgeState {
    Known,       // Present in store, not yet verified this session
    Validated,   // Successfully communicated with or passively verified recently
    Stale,       // Not seen for a while, requires active discovery
    Unavailable, // Confirmed offline or unreachable
    Archived     // Marked as forgotten by the user/system
};

struct KnowledgeConfidence {
    int score{0}; // 0-100 metric derived from frequency, validation, history
    KnowledgeState computedState{KnowledgeState::Known}; 
};

/**
 * @brief Rich history of an execution attempt.
 */
struct CommunicationRecord {
    std::string transportName;
    long long timestamp{0};
    int durationMs{0};
    ExecutionStatus status{ExecutionStatus::ExecutionFailed};
    std::string failureClassification;
};

/**
 * @brief Categorization of lifecycle events.
 */
enum class JournalEventType { 
    Discovered, 
    Validated, 
    CommSucceeded, 
    CommFailed, 
    AliasAdded, 
    Archived, 
    NetworkChanged 
};

/**
 * @brief A lightweight chronologically preserved event.
 */
struct JournalEntry {
    long long timestamp{0};
    JournalEventType type{JournalEventType::Discovered};
    std::string description;
};

/**
 * @brief Distinguishes between system-learned identities and user-assigned names.
 */
struct EntityAliases {
    std::vector<std::string> systemAliases; // Learned from UPnP, mDNS, etc.
    std::vector<std::string> userAliases;   // Assigned by humans
};

} // namespace NetDiscovery
