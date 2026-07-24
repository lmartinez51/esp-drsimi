/**
 * @file MQTTAdapter.h
 * @brief Persistent-session MQTT Protocol Adapter implementing IProtocolAdapter (v5.0.0 Architecture Phase 13).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"
#include "protocol/connection/IConnectionManager.h"
#include "protocol/session/IProtocolSessionManager.h"
#include "protocol/capability/ProtocolCapabilitySet.h"
#include "protocol/mqtt/IMQTTTransport.h"
#include "protocol/mqtt/MQTTBrokerContext.h"
#include "protocol/mqtt/MQTTSessionContext.h"
#include "protocol/mqtt/MQTTMessageBuilder.h"
#include "protocol/mqtt/MQTTResponseMapper.h"
#include "protocol/mqtt/MQTTRetryPolicy.h"
#include "protocol/mqtt/MQTTAdapterStatistics.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <optional>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Reference implementation of IProtocolAdapter for persistent MQTT connections.
 */
class MQTTAdapter : public IProtocolAdapter {
public:
    explicit MQTTAdapter(ProtocolAdapterDescriptor descriptor        = DefaultDescriptor(),
                         IMQTTTransport*           transport         = nullptr,
                         IConnectionManager*       connectionManager = nullptr,
                         IProtocolSessionManager*  sessionManager    = nullptr,
                         MQTTRetryPolicy           retryPolicy       = MQTTRetryPolicy());

    ~MQTTAdapter() override = default;

    static ProtocolAdapterDescriptor DefaultDescriptor();

    void SetConnectionManager(IConnectionManager* connectionManager);
    void SetSessionManager(IProtocolSessionManager* sessionManager);
    void SetTransport(IMQTTTransport* transport);

    // ── Broker Context Registration ─────────────────────────────────────────
    void RegisterBrokerContext(MQTTBrokerContext broker);

    bool HasBrokerContext(const std::string& brokerUri) const;
    std::optional<MQTTBrokerContext> GetBrokerContext(const std::string& brokerUri) const;

    // ── Telemetry ───────────────────────────────────────────────────────────
    const MQTTAdapterStatistics& GetStatistics() const { return m_stats; }

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
    IMQTTTransport*           m_transport{nullptr};
    IConnectionManager*       m_connectionManager{nullptr};
    IProtocolSessionManager*  m_sessionManager{nullptr};
    MQTTRetryPolicy           m_retryPolicy;
    bool                      m_isInitialized{false};
    bool                      m_isAvailable{false};

    MQTTMessageBuilder        m_builder;
    MQTTResponseMapper        m_mapper;
    MQTTAdapterStatistics     m_stats;

    mutable std::mutex        m_contextMutex;
    std::unordered_map<std::string, MQTTBrokerContext> m_brokers;
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
