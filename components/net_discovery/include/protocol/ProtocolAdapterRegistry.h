/**
 * @file ProtocolAdapterRegistry.h
 * @brief Thread-safe O(1) registry owning protocol adapter registrations (v5.0.0 Architecture Phase 10).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"
#include "protocol/ProtocolAdapterDescriptor.h"

#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>
#include <string>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Thread-safe registry owning shared_ptr<IProtocolAdapter> entries.
 *
 * RuntimeExecutionEngine receives a pointer to this registry at construction.
 * No singleton. No global state. Fully injectable.
 *
 * Lookup by adapterId is O(1). Queries by protocol or operation are O(n).
 */
class ProtocolAdapterRegistry {
public:
    ProtocolAdapterRegistry() = default;
    ~ProtocolAdapterRegistry() = default;

    // Non-copyable
    ProtocolAdapterRegistry(const ProtocolAdapterRegistry&) = delete;
    ProtocolAdapterRegistry& operator=(const ProtocolAdapterRegistry&) = delete;

    // ── Write Operations ────────────────────────────────────────────────────

    /**
     * @brief Registers an adapter. Replaces any existing entry with the same adapterId.
     */
    void Register(std::shared_ptr<IProtocolAdapter> adapter);

    /**
     * @brief Removes an adapter by ID.
     * @return true if the adapter was found and removed.
     */
    bool Remove(const AdapterId& adapterId);

    // ── Lookup ──────────────────────────────────────────────────────────────

    /**
     * @brief Returns the adapter for the given ID. O(1). Returns nullptr if not found.
     */
    std::shared_ptr<IProtocolAdapter> Find(const AdapterId& adapterId) const;

    /**
     * @brief Returns true if an adapter with the given ID is registered.
     */
    bool Contains(const AdapterId& adapterId) const;

    /**
     * @brief Returns the current count of registered adapters.
     */
    std::size_t Count() const;

    // ── Queries ─────────────────────────────────────────────────────────────

    /**
     * @brief Returns all adapters whose descriptor.protocolName matches the given protocol.
     */
    std::vector<std::shared_ptr<IProtocolAdapter>> FindByProtocol(const std::string& protocolName) const;

    /**
     * @brief Returns all adapters that declare support for the given operationId.
     */
    std::vector<std::shared_ptr<IProtocolAdapter>> FindByOperation(const std::string& operationId) const;

    /**
     * @brief Returns all adapters that declare support for the given capability tag.
     */
    std::vector<std::shared_ptr<IProtocolAdapter>> FindByCapability(const std::string& capability) const;

    /**
     * @brief Returns all adapters currently in Available state.
     */
    std::vector<std::shared_ptr<IProtocolAdapter>> GetAvailableAdapters() const;

    // ── Enumeration ─────────────────────────────────────────────────────────

    /**
     * @brief Returns a snapshot of all registered AdapterIds.
     */
    std::vector<AdapterId> GetAllAdapterIds() const;

    /**
     * @brief Returns a snapshot of all registered adapter descriptors (immutable metadata).
     */
    std::vector<ProtocolAdapterDescriptor> GetAllDescriptors() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<AdapterId, std::shared_ptr<IProtocolAdapter>> m_adapters;
};

} // namespace Protocol
} // namespace NetDiscovery
