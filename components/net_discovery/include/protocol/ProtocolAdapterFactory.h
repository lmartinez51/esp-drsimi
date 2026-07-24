/**
 * @file ProtocolAdapterFactory.h
 * @brief Open-closed factory for creating IProtocolAdapter instances without exposing concrete types (v5.0.0 Architecture Phase 10).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Creator function type: a zero-argument callable returning a new adapter instance.
 */
using AdapterCreator = std::function<std::shared_ptr<IProtocolAdapter>()>;

/**
 * @brief Open-closed factory for protocol adapters.
 *
 * Callers register a creator function keyed by protocol name. The factory creates
 * adapter instances on demand without ever importing concrete adapter headers.
 *
 * To add a new protocol (e.g., Zigbee, KNX):
 *   1. Implement a class derived from IProtocolAdapter.
 *   2. Call factory.RegisterCreator("zigbee", []{ return std::make_shared<ZigbeeAdapter>(); });
 *   3. No modifications to RuntimeExecutionEngine, ExecutionPlanner, or any semantic component.
 *
 * No reflection. No macros. No global registration tables.
 */
class ProtocolAdapterFactory {
public:
    ProtocolAdapterFactory() = default;
    ~ProtocolAdapterFactory() = default;

    // Non-copyable — factory owns registration table
    ProtocolAdapterFactory(const ProtocolAdapterFactory&) = delete;
    ProtocolAdapterFactory& operator=(const ProtocolAdapterFactory&) = delete;

    // ── Registration ────────────────────────────────────────────────────────

    /**
     * @brief Associates a creator function with a protocol name.
     *
     * Replaces any existing creator for the same protocol name.
     *
     * @param protocolName  Key used to select the creator (case-sensitive).
     * @param creator       Zero-arg callable returning a new adapter instance.
     */
    void RegisterCreator(const std::string& protocolName, AdapterCreator creator);

    /**
     * @brief Removes the creator for the given protocol name.
     * @return true if a creator was found and removed.
     */
    bool UnregisterCreator(const std::string& protocolName);

    /**
     * @brief Returns true if a creator is registered for the given protocol name.
     */
    bool HasCreator(const std::string& protocolName) const;

    // ── Creation ────────────────────────────────────────────────────────────

    /**
     * @brief Creates a new adapter instance for the given protocol name.
     *
     * @return Newly constructed adapter, or nullptr if no creator is registered.
     */
    std::shared_ptr<IProtocolAdapter> Create(const std::string& protocolName) const;

    /**
     * @brief Returns a list of all registered protocol names.
     */
    std::vector<std::string> GetRegisteredProtocols() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, AdapterCreator> m_creators;
};

} // namespace Protocol
} // namespace NetDiscovery
