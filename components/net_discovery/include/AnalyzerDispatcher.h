/**
 * @file AnalyzerDispatcher.h
 * @brief Routes Packets to all registered protocol analyzers.
 *
 * Dispatcher layer — sits between main() and the protocol analyzers.
 *
 * main() never calls SSDPAnalyzer (or any future analyzer) directly.
 * Instead it calls AnalyzerDispatcher::Dispatch(), which in turn
 * invokes every registered analyzer in registration order.
 *
 * Adding a new protocol analyzer requires zero changes to main() or
 * to any existing analyzer:
 *
 * @code
 *   AnalyzerDispatcher dispatcher;
 *   dispatcher.Register(std::make_unique<SSDPAnalyzer>());
 *   dispatcher.Register(std::make_unique<HTTPAnalyzer>());  // future
 *
 *   dispatcher.Dispatch(packet, registry);
 * @endcode
 *
 * Design notes:
 *   - Ownership: uses unique_ptr so each analyzer has a single owner.
 *   - Order:  analyzers are invoked in registration order.
 *   - Errors: a throwing analyzer is caught, logged, and skipped;
 *             other analyzers in the chain still execute.
 *   - No routing by ProtocolType at this stage (all packets go to all
 *     analyzers). Future: add a routing table keyed by ProtocolType.
 *
 * Portability: Depends only on IProtocolAnalyzer and Core — fully
 * portable to ESP-IDF (assuming std::unique_ptr is available, which
 * it is in ESP-IDF v5+ with the bundled libstdc++).
 */

#pragma once

#include "IProtocolAnalyzer.h"

#include <memory>
#include <string>
#include <vector>

namespace NetDiscovery {

class DeviceRegistry;

// ============================================================
// AnalyzerDispatcher
// ============================================================

/**
 * @brief Dispatches received Packets to all registered protocol analyzers.
 */
class AnalyzerDispatcher {
public:
    AnalyzerDispatcher() = default;

    // Non-copyable — owns unique_ptr analyzers.
    AnalyzerDispatcher(const AnalyzerDispatcher&) = delete;
    AnalyzerDispatcher& operator=(const AnalyzerDispatcher&) = delete;

    // Movable.
    AnalyzerDispatcher(AnalyzerDispatcher&&) noexcept = default;
    AnalyzerDispatcher& operator=(AnalyzerDispatcher&&) noexcept = default;

    // ----------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------

    /**
     * @brief Register a protocol analyzer.
     *
     * Takes ownership of @p analyzer. Analyzers are invoked in the
     * order they were registered.
     *
     * @param analyzer  Concrete analyzer instance (must not be null).
     * @throws std::invalid_argument if analyzer is null.
     */
    void Register(std::unique_ptr<IProtocolAnalyzer> analyzer);

    // ----------------------------------------------------------------
    // Dispatch
    // ----------------------------------------------------------------

    /**
     * @brief Send a Packet to all registered analyzers in order.
     *
     * Each analyzer's Analyze() is called. If an analyzer throws, the
     * exception is caught, a warning is printed to stderr, and the
     * next analyzer is invoked.
     *
     * @param packet    Packet to analyze (passed by const ref).
     * @param registry  Registry that analyzers may update.
     */
    void Dispatch(const Packet& packet, DeviceRegistry& registry) const;

    // ----------------------------------------------------------------
    // Inspection
    // ----------------------------------------------------------------

    /**
     * @brief Return the Name() of all registered analyzers in order.
     */
    std::vector<std::string> GetAnalyzerNames() const;

    /**
     * @brief Return the number of registered analyzers.
     */
    std::size_t AnalyzerCount() const noexcept;

private:
    std::vector<std::unique_ptr<IProtocolAnalyzer>> m_analyzers;
};

} // namespace NetDiscovery
