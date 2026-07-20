/**
 * @file DeviceRegistry.h
 * @brief In-memory registry of all discovered evidence.
 *
 * Central repository populated by protocol analyzers.
 * Thread-safe via std::mutex.
 */

#pragma once

#include "core/evidence/IdentityEvidence.h"

#include <mutex>
#include <vector>

namespace NetDiscovery {

class DeviceRegistry {
public:
    DeviceRegistry() = default;

    // Non-copyable (owns unique state).
    DeviceRegistry(const DeviceRegistry&) = delete;
    DeviceRegistry& operator=(const DeviceRegistry&) = delete;

    // Movable.
    DeviceRegistry(DeviceRegistry&&) noexcept = default;
    DeviceRegistry& operator=(DeviceRegistry&&) noexcept = default;

    /**
     * @brief Register evidence. Deduplicates identical packets.
     * @param evidence  IdentityEvidence to register.
     */
    void Register(const IdentityEvidence& evidence);

    /**
     * @brief Remove all evidence from the registry.
     */
    void Clear();

    /**
     * @brief Return a snapshot of all registered evidence.
     * @return  Vector of all IdentityEvidence objects.
     */
    std::vector<IdentityEvidence> GetAll() const;

    /**
     * @brief Get all evidence that represents an SSDP announcement lacking full description.
     */
    std::vector<IdentityEvidence> GetEvidencePendingDescription() const;

    /**
     * @brief Return the number of unique evidence pieces currently registered.
     */
    std::size_t Size() const noexcept;

private:
    mutable std::mutex m_mutex;
    std::vector<IdentityEvidence> m_evidence;
};

} // namespace NetDiscovery
