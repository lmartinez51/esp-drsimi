/**
 * @file HTTPAdapter.h
 * @brief Decoupled REST HTTP Protocol Adapter implementing IProtocolAdapter (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"
#include "protocol/connection/IConnectionManager.h"
#include "protocol/session/IProtocolSessionManager.h"
#include "protocol/capability/ProtocolCapabilitySet.h"
#include "protocol/http/IHTTPTransport.h"
#include "protocol/http/HTTPDeviceContext.h"
#include "protocol/http/HTTPSessionContext.h"
#include "protocol/http/HTTPRequestBuilder.h"
#include "protocol/http/HTTPResponseParser.h"
#include "protocol/http/HTTPResponseMapper.h"
#include "protocol/http/HTTPRetryPolicy.h"
#include "protocol/http/HTTPAdapterStatistics.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <optional>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Reference implementation of IProtocolAdapter for REST/HTTP device integrations.
 *
 * Functions strictly as an orchestrator across isolated sub-components:
 *   - Request building:     HTTPRequestBuilder
 *   - Connection leasing:   IConnectionManager
 *   - Session leasing:      IProtocolSessionManager
 *   - Transport execution:  IHTTPTransport
 *   - Response parsing:     HTTPResponseParser
 *   - Response mapping:     HTTPResponseMapper
 *   - Retry logic:          HTTPRetryPolicy
 *   - Statistics:           HTTPAdapterStatistics
 *
 * Owns NO socket handles, NO retry loops, NO JSON parsers, and NO connection pools.
 */
class HTTPAdapter : public IProtocolAdapter {
public:
    explicit HTTPAdapter(ProtocolAdapterDescriptor descriptor        = DefaultDescriptor(),
                         IHTTPTransport*           transport         = nullptr,
                         IConnectionManager*       connectionManager = nullptr,
                         IProtocolSessionManager*  sessionManager    = nullptr,
                         HTTPRetryPolicy           retryPolicy       = HTTPRetryPolicy());

    ~HTTPAdapter() override = default;

    static ProtocolAdapterDescriptor DefaultDescriptor();

    void SetConnectionManager(IConnectionManager* connectionManager);
    void SetSessionManager(IProtocolSessionManager* sessionManager);
    void SetTransport(IHTTPTransport* transport);

    // ── Device Context Registration ─────────────────────────────────────────
    void RegisterDeviceContext(HTTPDeviceContext device);

    bool HasDeviceContext(const std::string& baseUrl) const;
    std::optional<HTTPDeviceContext> GetDeviceContext(const std::string& baseUrl) const;

    // ── Telemetry ───────────────────────────────────────────────────────────
    const HTTPAdapterStatistics& GetStatistics() const { return m_stats; }

    // ── IProtocolAdapter ────────────────────────────────────────────────────
    bool Initialize() override;
    void Shutdown() override;
    bool IsAvailable() const override;

    const ProtocolAdapterDescriptor& GetDescriptor() const override;
    Runtime::DispatcherCapabilities GetCapabilities() const override;
    ProtocolCapabilitySet GetProtocolCapabilities() const override;

    Runtime::ExecutionStepResult Execute(
        const Execution::ExecutionStep&        step,
        Execution::ExecutionSession&           session,
        Runtime::ExecutionRuntimeContext&      context) override;

private:
    ProtocolAdapterDescriptor m_descriptor;
    IHTTPTransport*           m_transport{nullptr};
    IConnectionManager*       m_connectionManager{nullptr};
    IProtocolSessionManager*  m_sessionManager{nullptr};
    HTTPRetryPolicy           m_retryPolicy;
    bool                      m_isInitialized{false};
    bool                      m_isAvailable{false};

    HTTPRequestBuilder        m_builder;
    HTTPResponseParser        m_parser;
    HTTPResponseMapper        m_mapper;
    HTTPAdapterStatistics     m_stats;

    mutable std::mutex        m_contextMutex;
    std::unordered_map<std::string, HTTPDeviceContext> m_devices;
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
