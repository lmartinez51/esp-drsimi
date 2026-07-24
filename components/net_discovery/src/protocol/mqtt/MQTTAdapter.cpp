/**
 * @file MQTTAdapter.cpp
 * @brief Implementation of orchestrator-only MQTTAdapter (v5.0.0 Architecture Phase 12.1).
 */

#include "protocol/mqtt/MQTTAdapter.h"

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

ProtocolAdapterDescriptor MQTTAdapter::DefaultDescriptor() {
    return ProtocolAdapterDescriptor(
        "adapter.mqtt.default",
        "MQTT",
        "1.1.0",
        "ESP-Claw Architecture",
        "TCP/TLS",
        {"Publish", "Subscribe", "Unsubscribe", "Disconnect"},
        {"pubsub", "telemetry", "events", "persistent_session"},
        {{"protocol_version", "3.1.1"}, {"architecture", "v5.0.0 Phase 12.1"}}
    );
}

MQTTAdapter::MQTTAdapter(ProtocolAdapterDescriptor descriptor,
                         IMQTTTransport* transport,
                         IConnectionManager* connectionManager,
                         IProtocolSessionManager* sessionManager,
                         MQTTRetryPolicy retryPolicy)
    : m_descriptor(std::move(descriptor))
    , m_transport(transport)
    , m_connectionManager(connectionManager)
    , m_sessionManager(sessionManager)
    , m_retryPolicy(std::move(retryPolicy)) {}

void MQTTAdapter::SetConnectionManager(IConnectionManager* connectionManager) {
    m_connectionManager = connectionManager;
}

void MQTTAdapter::SetSessionManager(IProtocolSessionManager* sessionManager) {
    m_sessionManager = sessionManager;
}

void MQTTAdapter::SetTransport(IMQTTTransport* transport) {
    m_transport = transport;
}

void MQTTAdapter::RegisterBrokerContext(MQTTBrokerContext broker) {
    if (!broker.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_contextMutex);
    const std::string key = broker.brokerUri.empty() ? broker.hostname : broker.brokerUri;
    m_brokers.insert_or_assign(key, std::move(broker));
}

bool MQTTAdapter::HasBrokerContext(const std::string& brokerUri) const {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    return m_brokers.count(brokerUri) > 0;
}

std::optional<MQTTBrokerContext> MQTTAdapter::GetBrokerContext(const std::string& brokerUri) const {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    auto it = m_brokers.find(brokerUri);
    if (it == m_brokers.end()) return std::nullopt;
    return it->second;
}

bool MQTTAdapter::Initialize() {
    m_isInitialized = true;
    m_isAvailable   = true;
    return true;
}

void MQTTAdapter::Shutdown() {
    m_isAvailable   = false;
    m_isInitialized = false;
}

bool MQTTAdapter::IsAvailable() const {
    return m_isInitialized && m_isAvailable;
}

const ProtocolAdapterDescriptor& MQTTAdapter::GetDescriptor() const {
    return m_descriptor;
}

Runtime::DispatcherCapabilities MQTTAdapter::GetCapabilities() const {
    Runtime::DispatcherCapabilities caps;
    caps.supportsCancellation   = true;
    caps.supportsTimeouts       = true;
    caps.supportsBatchExecution = false;
    caps.supportsRollback       = false;
    return caps;
}

ProtocolCapabilitySet MQTTAdapter::GetProtocolCapabilities() const {
    ProtocolCapabilitySet caps;
    caps.AddCapability(ProtocolCapability("mqtt.publish", "MQTT Publish", "Supports publishing messages"));
    caps.AddCapability(ProtocolCapability("mqtt.subscribe", "MQTT Subscribe", "Supports topic subscriptions"));
    caps.AddCapability(ProtocolCapability("mqtt.qos0", "MQTT QoS 0", "At most once delivery"));
    caps.AddCapability(ProtocolCapability("mqtt.qos1", "MQTT QoS 1", "At least once delivery"));
    caps.AddCapability(ProtocolCapability("mqtt.persistent_sessions", "MQTT Persistent Sessions", "Supports clean session false / persistent sessions"));
    return caps;
}


Runtime::ExecutionStepResult MQTTAdapter::Execute(
        const Execution::ExecutionStep&        step,
        Execution::ExecutionSession&           /*session*/,
        Runtime::ExecutionRuntimeContext&      /*context*/) {

    m_stats.RecordRequest();

    if (!IsAvailable()) {
        m_stats.RecordFailure();
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::Failure,
            m_descriptor.adapterId, 0, 0, -1,
            "MQTTAdapter: adapter is not initialized or unavailable",
            false, false, {}, {},
            {{"adapterId", m_descriptor.adapterId}});
    }

    if (!m_transport) {
        m_stats.RecordFailure();
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::NotImplemented,
            m_descriptor.adapterId, 0, 0, -2,
            "MQTTAdapter: no IMQTTTransport attached",
            false, false, {}, {},
            {{"adapterId", m_descriptor.adapterId}});
    }

    // 1. Resolve broker context
    MQTTBrokerContext broker;
    {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        if (!m_brokers.empty()) {
            broker = m_brokers.begin()->second;
        } else {
            broker = MQTTBrokerContext("mqtt://localhost:1883", "localhost", 1883);
        }
    }

    // 2. Build MQTTRequest from ExecutionStep
    MQTTRequest request = m_builder.BuildRequest(step);

    switch (request.operationType) {
        case MQTTOperationType::Publish:     m_stats.RecordPublish(); break;
        case MQTTOperationType::Subscribe:   m_stats.RecordSubscribe(); break;
        case MQTTOperationType::Unsubscribe: m_stats.RecordUnsubscribe(); break;
        case MQTTOperationType::Disconnect:  m_stats.RecordDisconnect(); break;
    }

    // 3. Acquire ProtocolSession from IProtocolSessionManager
    std::optional<ProtocolSession> pSession;
    if (m_sessionManager) {
        pSession = m_sessionManager->AcquireSession(
            m_descriptor.adapterId, "MQTT", broker.brokerUri, step.GetTimeoutMs());
    }

    MQTTSessionContext sessionCtx("esp_claw_default");

    // 4. Lease ConnectionHandle from IConnectionManager
    std::optional<ConnectionHandle> connectionHandle;
    if (m_connectionManager) {
        connectionHandle = m_connectionManager->AcquireConnection(
            "MQTT", broker.brokerUri, step.GetTimeoutMs());
    }

    ConnectionHandle activeHandle = connectionHandle.has_value() ? connectionHandle.value() : ConnectionHandle("conn.mqtt.default", "MQTT", broker.brokerUri);

    // 5. Execute via IMQTTTransport with retry policy evaluation
    uint32_t attempt = 0;
    MQTTResponse response;

    while (true) {
        response = m_transport->Send(request, sessionCtx, activeHandle);

        if (response.IsSuccess()) break;

        if (m_retryPolicy.ShouldRetry(response, attempt)) {
            m_stats.RecordRetry();
            ++attempt;
        } else {
            break;
        }
    }

    // Release leased ConnectionHandle back to IConnectionManager
    if (m_connectionManager && connectionHandle.has_value()) {
        m_connectionManager->ReleaseConnection(activeHandle);
    }

    // Release leased ProtocolSession back to IProtocolSessionManager
    if (m_sessionManager && pSession.has_value()) {
        m_sessionManager->ReleaseSession(pSession.value());
    }

    m_stats.RecordBytes(response.bytesTransmitted, response.bytesReceived);
    if (response.IsTimeout()) {
        m_stats.RecordTransportError();
    }

    // 6. Map outcome to ExecutionStepResult
    Runtime::ExecutionStepResult stepResult = m_mapper.MapToStepResult(
        step.GetStepId(), m_descriptor.adapterId, response);

    if (stepResult.IsSuccess()) {
        m_stats.RecordSuccess();
    } else {
        m_stats.RecordFailure();
        if (!response.IsSuccess() && !response.IsTimeout()) {
            m_stats.RecordProtocolError();
        }
    }

    m_stats.RecordLatency(response.elapsedTimeMs);

    return stepResult;
}

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
