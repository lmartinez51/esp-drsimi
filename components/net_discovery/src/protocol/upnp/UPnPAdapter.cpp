/**
 * @file UPnPAdapter.cpp
 * @brief Implementation of orchestrator-only UPnPAdapter (v5.0.0 Architecture Phase 12.1).
 */

#include "protocol/upnp/UPnPAdapter.h"

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

ProtocolAdapterDescriptor UPnPAdapter::DefaultDescriptor() {
    return ProtocolAdapterDescriptor(
        "adapter.upnp.default",
        "UPnP",
        "1.3.0",
        "ESP-Claw Architecture",
        "HTTP/SOAP",
        {"Play", "Pause", "Stop", "Next", "Previous", "SetVolume", "GetVolume", "SetMute", "GetMute", "SetAVTransportURI"},
        {"media", "rendering_control", "av_transport", "connection_manager"},
        {{"protocol_version", "1.3"}, {"architecture", "v5.0.0 Phase 12.1"}}
    );
}

UPnPAdapter::UPnPAdapter(ProtocolAdapterDescriptor descriptor,
                         IUPnPTransport* transport,
                         IConnectionManager* connectionManager,
                         IProtocolSessionManager* sessionManager,
                         UPnPRetryPolicy retryPolicy)
    : m_descriptor(std::move(descriptor))
    , m_transport(transport)
    , m_connectionManager(connectionManager)
    , m_sessionManager(sessionManager)
    , m_retryPolicy(std::move(retryPolicy))
    , m_requestBuilder(m_serializer) {}

void UPnPAdapter::SetConnectionManager(IConnectionManager* connectionManager) {
    m_connectionManager = connectionManager;
}

void UPnPAdapter::SetSessionManager(IProtocolSessionManager* sessionManager) {
    m_sessionManager = sessionManager;
}

void UPnPAdapter::RegisterDeviceContext(UPnPDeviceContext device) {
    if (!device.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_contextMutex);
    const std::string udn = device.udn;
    m_devices.insert_or_assign(udn, std::move(device));
}

bool UPnPAdapter::HasDeviceContext(const std::string& udn) const {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    return m_devices.count(udn) > 0;
}

std::optional<UPnPDeviceContext> UPnPAdapter::GetDeviceContext(const std::string& udn) const {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    auto it = m_devices.find(udn);
    if (it == m_devices.end()) return std::nullopt;
    return it->second;
}

bool UPnPAdapter::Initialize() {
    m_isInitialized = true;
    m_isAvailable   = true;
    return true;
}

void UPnPAdapter::Shutdown() {
    m_isAvailable   = false;
    m_isInitialized = false;
}

bool UPnPAdapter::IsAvailable() const {
    return m_isInitialized && m_isAvailable;
}

const ProtocolAdapterDescriptor& UPnPAdapter::GetDescriptor() const {
    return m_descriptor;
}

Runtime::DispatcherCapabilities UPnPAdapter::GetCapabilities() const {
    Runtime::DispatcherCapabilities caps;
    caps.supportsCancellation   = true;
    caps.supportsTimeouts       = true;
    caps.supportsBatchExecution = false;
    caps.supportsRollback       = false;
    return caps;
}

ProtocolCapabilitySet UPnPAdapter::GetProtocolCapabilities() const {
    ProtocolCapabilitySet caps;
    caps.AddCapability(ProtocolCapability("upnp.soap", "UPnP SOAP Control", "Supports SOAP 1.1 control invocation"));
    caps.AddCapability(ProtocolCapability("upnp.eventing", "UPnP GENA Eventing", "Supports GENA event subscriptions"));
    caps.AddCapability(ProtocolCapability("upnp.service_resolution", "UPnP Service Resolution", "Supports SCPD and service resolution"));
    return caps;
}


Runtime::ExecutionStepResult UPnPAdapter::Execute(
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
            "UPnPAdapter: adapter is not initialized or unavailable",
            false, false, {}, {},
            {{"adapterId", m_descriptor.adapterId}});
    }

    if (!m_transport) {
        m_stats.RecordFailure();
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::NotImplemented,
            m_descriptor.adapterId, 0, 0, -2,
            "UPnPAdapter: no IUPnPTransport attached",
            false, false, {}, {},
            {{"adapterId", m_descriptor.adapterId}});
    }

    // 1. Resolve target device context
    std::string targetUdn;
    auto metaIt = step.GetMetadata().find("udn");
    if (metaIt != step.GetMetadata().end()) {
        targetUdn = metaIt->second;
    } else {
        auto paramIt = step.GetParameterValues().find("udn");
        if (paramIt != step.GetParameterValues().end()) {
            targetUdn = paramIt->second;
        }
    }

    UPnPDeviceContext device;
    {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        if (!targetUdn.empty()) {
            auto it = m_devices.find(targetUdn);
            if (it != m_devices.end()) device = it->second;
        }
        if (!device.IsValid() && !m_devices.empty()) {
            device = m_devices.begin()->second;
        }
    }

    if (!device.IsValid()) {
        m_stats.RecordFailure();
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::Failure,
            m_descriptor.adapterId, 0, 0, -3,
            "UPnPAdapter: no UPnPDeviceContext registered for execution",
            false, false, {}, {},
            {{"adapterId", m_descriptor.adapterId}});
    }

    // 2. Resolve target service
    std::string serviceHint;
    auto hintIt = step.GetMetadata().find("serviceType");
    if (hintIt != step.GetMetadata().end()) {
        serviceHint = hintIt->second;
    }

    auto serviceOpt = m_serviceResolver.ResolveService(device, step.GetOperationId(), serviceHint);

    if (!serviceOpt.has_value()) {
        m_stats.RecordFailure();
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::NotImplemented,
            m_descriptor.adapterId, 0, 0, -4,
            "UPnPAdapter: could not resolve service descriptor for operation '" + step.GetOperationId() + "'",
            false, false, {}, {},
            {{"operationId", step.GetOperationId()}});
    }

    // 3. Acquire ProtocolSession from IProtocolSessionManager (Phase 12.1 shared session infrastructure)
    std::optional<ProtocolSession> pSession;
    if (m_sessionManager) {
        pSession = m_sessionManager->AcquireSession(
            m_descriptor.adapterId, "UPnP", device.baseUrl, step.GetTimeoutMs());
    }

    UPnPSessionContext sessionCtx(device.udn);

    // 4. Translate parameters -> UPnPActionTranslation
    UPnPActionTranslation translation = m_translator.Translate(step);

    // 5. Build request -> UPnPRequest
    UPnPRequest request = m_requestBuilder.BuildRequest(
        translation, serviceOpt.value(), device.baseUrl, step.GetTimeoutMs());

    // 6. Acquire ConnectionHandle from IConnectionManager (Phase 11.2 connection infrastructure)
    std::optional<ConnectionHandle> connectionHandle;
    if (m_connectionManager) {
        connectionHandle = m_connectionManager->AcquireConnection(
            "UPnP", request.controlUrl, step.GetTimeoutMs());
    }

    ConnectionHandle activeHandle = connectionHandle.has_value() ? connectionHandle.value() : ConnectionHandle("conn.default", "UPnP", request.controlUrl);

    // 7. Send request via IUPnPTransport
    uint32_t attempt = 0;
    UPnPResponse response;

    while (true) {
        response = m_transport->Send(request, sessionCtx, activeHandle);

        if (response.IsSuccess()) break;

        if (m_retryPolicy.ShouldRetry(response, attempt)) {
            m_stats.RecordRetry();
            ++attempt;
            sessionCtx.retryCount = attempt;
        } else {
            break;
        }
    }

    // Release leased connection back to IConnectionManager
    if (m_connectionManager && connectionHandle.has_value()) {
        m_connectionManager->ReleaseConnection(activeHandle);
    }

    // Release leased ProtocolSession back to IProtocolSessionManager
    if (m_sessionManager && pSession.has_value()) {
        m_sessionManager->ReleaseSession(pSession.value());
    }

    if (response.IsTimeout()) {
        m_stats.RecordTransportError();
    }

    // 8. Parse response XML
    UPnPParsedResponse parsed = m_responseParser.Parse(response.xmlPayload, step.GetOperationId());

    if (!parsed.parseError.empty()) {
        m_stats.RecordParserError();
    }
    if (parsed.isFault) {
        m_stats.RecordSoapFault();
    }

    // 9. Map outcome -> ExecutionStepResult
    Runtime::ExecutionStepResult stepResult = m_errorMapper.MapToStepResult(
        step.GetStepId(), m_descriptor.adapterId, response, parsed);

    if (stepResult.IsSuccess()) {
        m_stats.RecordSuccess();
    } else {
        m_stats.RecordFailure();
    }

    m_stats.RecordLatency(response.elapsedTimeMs);

    return stepResult;
}

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
