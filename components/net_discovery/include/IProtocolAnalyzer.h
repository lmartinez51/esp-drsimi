/**
 * @file IProtocolAnalyzer.h
 * @brief Abstract interface for all protocol analyzers.
 *
 * Protocol layer — depends on Core (Packet), forward-declares DeviceRegistry.
 *
 * Every protocol implementation (SSDPAnalyzer, HTTPAnalyzer, SOAPAnalyzer,
 * SamsungAnalyzer, etc.) must inherit from this interface.
 *
 * Design rules:
 *   - Analyze() MUST NOT print anything. All output is handled by main().
 *   - Analyze() populates the DeviceRegistry and may update Packet::metadata.
 *   - Analyze() should be stateless where possible (prefer pure functions).
 *   - Name() returns a stable identifier string (e.g. "SSDP", "HTTP").
 *
 * Adding a new protocol:
 *   1. Create a class that inherits IProtocolAnalyzer.
 *   2. Implement Name() and Analyze().
 *   3. Register an instance with AnalyzerDispatcher.
 *   main() and all existing analyzers require zero modifications.
 *
 * Portability: Pure virtual class — fully portable to ESP-IDF.
 */

#pragma once

#include "core/Packet.h"

#include <string>

namespace NetDiscovery {

class DeviceRegistry;  // Forward declaration — breaks circular dependency.

// ============================================================
// IProtocolAnalyzer
// ============================================================

/**
 * @brief Pure abstract interface for protocol analysis plugins.
 *
 * Usage:
 * @code
 *   class SSDPAnalyzer : public IProtocolAnalyzer {
 *   public:
 *       std::string Name() const override { return "SSDP"; }
 *       void Analyze(const Packet& p, DeviceRegistry& r) override { ... }
 *   };
 * @endcode
 */
class IProtocolAnalyzer {
public:
    /**
     * @brief Return a stable, human-readable identifier for this analyzer.
     *
     * Used by AnalyzerDispatcher for logging and by callers to filter
     * registered analyzers by name.
     *
     * @return Protocol name string (e.g. "SSDP", "HTTP", "SOAP").
     */
    virtual std::string Name() const = 0;

    /**
     * @brief Analyze a single Packet and update the DeviceRegistry.
     *
     * Implementations MUST:
     *   - Be pure with respect to external I/O (no printing, no file writes).
     *   - Only call registry.Register() to persist results.
     *   - Not store the Packet reference beyond the duration of this call.
     *   - Tolerate packets from unrecognized protocols gracefully (no throw).
     *
     * Implementations MAY:
     *   - Write to packet.metadata for downstream analyzers.
     *   - Inspect packet.protocol and return early if not applicable.
     *
     * @param packet    The packet to analyze (read-only content).
     * @param registry  The registry to update with discovered devices.
     */
    virtual void Analyze(const Packet& packet, DeviceRegistry& registry) = 0;

    /**
     * @brief Virtual destructor — required for polymorphic deletion.
     */
    virtual ~IProtocolAnalyzer() = default;

    // Non-copyable — analyzers own internal state.
    IProtocolAnalyzer(const IProtocolAnalyzer&) = delete;
    IProtocolAnalyzer& operator=(const IProtocolAnalyzer&) = delete;

protected:
    IProtocolAnalyzer() = default;
};

} // namespace NetDiscovery
