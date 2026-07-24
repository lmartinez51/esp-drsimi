/**
 * @file CapabilitySet.h
 * @brief Lock-free capability container for KnowledgeEntity (v5.0.0 Architecture Phase 7.5).
 * 
 * ====================================================================================
 * SYNCHRONIZATION ARCHITECTURE & LOCK-FREE DESIGN JUSTIFICATION
 * ====================================================================================
 * CapabilitySet is intentionally lock-free and un-synchronized (owns no internal mutex).
 * 
 * CapabilitySet is never shared independently across thread boundaries. Its lifetime is strictly
 * owned by KnowledgeEntity, which is held as a value object. All concurrent reads, writes, and
 * updates to KnowledgeEntity instances occur inside thread-safe repositories 
 * (e.g., InMemoryKnowledgeRepository, FileKnowledgeRepository), where coarse-grained 
 * repository synchronization is already performed.
 * 
 * Removing internal mutexes eliminates redundant lock/unlock overhead, enables compiler-generated
 * copy/move constructors, and guarantees that CapabilitySet remains a lightweight, deterministic,
 * copyable, and movable value object.
 */

#pragma once

#include "core/CapabilityDefinition.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace NetDiscovery {

/**
 * @brief Lightweight, lock-free container managing semantic capabilities owned by a KnowledgeEntity.
 */
class CapabilitySet {
public:
    CapabilitySet() = default;
    ~CapabilitySet() = default;

    // Pure value semantics (copyable and movable without mutex locks)
    CapabilitySet(const CapabilitySet& other) = default;
    CapabilitySet& operator=(const CapabilitySet& other) = default;
    CapabilitySet(CapabilitySet&& other) noexcept = default;
    CapabilitySet& operator=(CapabilitySet&& other) noexcept = default;
    CapabilitySet& operator=(const std::vector<CapabilityModel>& vecCapabilities);

    // Mutation Operations
    bool AddCapability(const CapabilityModel& cap);
    bool RemoveCapability(const std::string& capId);
    void MergeCapabilities(const CapabilitySet& other);
    void Clear();

    // Query Operations (O(1) Average Lookup)
    bool HasCapability(const std::string& capId) const;
    std::optional<CapabilityModel> FindCapability(const std::string& capId) const;
    std::vector<CapabilityModel> GetCapabilities() const;
    size_t Size() const { return m_capabilities.size(); }
    bool Empty() const { return m_capabilities.empty(); }

private:
    std::unordered_map<std::string, CapabilityModel> m_capabilities;
};

} // namespace NetDiscovery
