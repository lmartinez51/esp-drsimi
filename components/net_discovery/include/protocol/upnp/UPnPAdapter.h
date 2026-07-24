/**
 * @file UPnPAdapter.h
 * @brief Reference UPnP Protocol Adapter implementing IProtocolAdapter (v5.0.0 Architecture Phase 13).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"
#include "protocol/connection/IConnectionManager.h"
#include "protocol/session/IProtocolSessionManager.h"
#include "protocol/capability/ProtocolCapabilitySet.h"
#include "protocol/upnp/IUPnPTransport.h"
#include "protocol/upnp/UPnPDeviceContext.h"
#include "protocol/upnp/UPnPSessionContext.h"
#include "protocol/upnp/UPnPActionTranslator.h"
#include "protocol/upnp/UPnPRequestBuilder.h"
#include "protocol/upnp/UPnPSoapSerializer.h"
#include "protocol/upnp/UPnPServiceResolver.h"
#include "protocol/upnp/UPnPResponseParser.h"
#include "protocol/upnp/UPnPErrorMapper.h"
#include "protocol/upnp/UPnPRetryPolicy.h"
#include "protocol/upnp/UPnPAdapterStatistics.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Reference implementation of IProtocolAdapter for UPnP devices.
 */
class UPnPAdapter : public IProtocolAdapter {
public:
    explicit UPnPAdapter(ProtocolAdapterDescriptor descriptor        = DefaultDescriptor(),
                         IUPnPTransport*           transport         = nullptr,
                         IConnectionManager*       connectionManager = nullptr,
                         IProtocolSessionManager*  sessionManager    = nullptr,
                         UPnPRetryPolicy           retryPolicy       = UPnPRetryPolicy());

    ~UPnPAdapter() override = default;

    static ProtocolAdapterDescriptor DefaultDescriptor();

    void SetConnectionManager(IConnectionManager* connectionManager);
    void SetSessionManager(IProtocolSessionManager* sessionManager);

    // ── Device Context Registration ─────────────────────────────────────────
    void RegisterDeviceContext(UPnPDeviceContext device);

    bool HasDeviceContext(const std::string& udn) const;
    std::optional<UPnPDeviceContext> GetDeviceContext(const std::string& udn) const;

    // ── Telemetry ───────────────────────────────────────────────────────────
    const UPnPAdapterStatistics& GetStatistics() const { return m_stats; }

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
    IUPnPTransport*           m_transport{nullptr};
    IConnectionManager*       m_connectionManager{nullptr};
    IProtocolSessionManager*  m_sessionManager{nullptr};
    UPnPRetryPolicy           m_retryPolicy;
    bool                      m_isInitialized{false};
    bool                      m_isAvailable{false};

    UPnPActionTranslator      m_translator;
    UPnPSoapSerializer        m_serializer;
    UPnPRequestBuilder        m_requestBuilder;
    UPnPServiceResolver       m_serviceResolver;
    UPnPResponseParser        m_responseParser;
    UPnPErrorMapper           m_errorMapper;

    UPnPAdapterStatistics     m_stats;

    mutable std::mutex        m_contextMutex;
    std::unordered_map<std::string, UPnPDeviceContext> m_devices;
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
