/**
 * @file HTTPAdapter.cpp
 * @brief Implementation of orchestrator-only HTTPAdapter (v5.0.0 Architecture Phase 15).
 */

#include "protocol/http/HTTPAdapter.h"

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

ProtocolAdapterDescriptor HTTPAdapter::DefaultDescriptor() {
    return ProtocolAdapterDescriptor(
        "adapter.http.default",
        "HTTP",
        "1.0.0",
        "ESP-Claw Architecture",
        "HTTP/REST",
        {"GET", "POST", "PUT", "DELETE", "PATCH"},
        {"rest", "http", "api", "shelly", "esphome", "tasmota"},
        {{"protocol_version", "1.1"}, {"architecture", "v5.0.0 Phase 15"}}
    );
}

HTTPAdapter::HTTPAdapter(ProtocolAdapterDescriptor descriptor,
                         IHTTPTransport* transport,
                         IConnectionManager* connectionManager,
                         IProtocolSessionManager* sessionManager,
                         HTTPRetryPolicy retryPolicy)
    : m_descriptor(std::move(descriptor))
    , m_transport(transport)
    , m_connectionManager(connectionManager)
    , m_sessionManager(sessionManager)
    , m_retryPolicy(std::move(retryPolicy)) {}

void HTTPAdapter::SetConnectionManager(IConnectionManager* connectionManager) {
    m_connectionManager = connectionManager;
}

void HTTPAdapter::SetSessionManager(IProtocolSessionManager* sessionManager) {
    m_sessionManager = sessionManager;
}

void HTTPAdapter::SetTransport(IHTTPTransport* transport) {
    m_transport = transport;
}

void HTTPAdapter::RegisterDeviceContext(HTTPDeviceContext device) {
    if (!device.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_contextMutex);
    const std::string key = device.baseUrl.empty() ? device.hostname : device.baseUrl;
    m_devices.insert_or_assign(key, std::move(device));
}

bool HTTPAdapter::HasDeviceContext(const std::string& baseUrl) const {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    return m_devices.count(baseUrl) > 0;
}

std::optional<HTTPDeviceContext> HTTPAdapter::GetDeviceContext(const std::string& baseUrl) const {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    auto it = m_devices.find(baseUrl);
    if (it == m_devices.end()) return std::nullopt;
    return it->second;
}

bool HTTPAdapter::Initialize() {
    m_isInitialized = true;
    m_isAvailable   = true;
    return true;
}

void HTTPAdapter::Shutdown() {
    m_isAvailable   = false;
    m_isInitialized = false;
}

bool HTTPAdapter::IsAvailable() const {
    return m_isInitialized && m_isAvailable;
}

const ProtocolAdapterDescriptor& HTTPAdapter::GetDescriptor() const {
    return m_descriptor;
}

Runtime::DispatcherCapabilities HTTPAdapter::GetCapabilities() const {
    Runtime::DispatcherCapabilities caps;
    caps.supportsCancellation   = true;
    caps.supportsTimeouts       = true;
    caps.supportsBatchExecution = false;
    caps.supportsRollback       = false;
    return caps;
}

ProtocolCapabilitySet HTTPAdapter::GetProtocolCapabilities() const {
    ProtocolCapabilitySet caps;
    caps.AddCapability(ProtocolCapability("http.get", "HTTP GET", "Supports HTTP GET requests"));
    caps.AddCapability(ProtocolCapability("http.post", "HTTP POST", "Supports HTTP POST requests"));
    caps.AddCapability(ProtocolCapability("http.put", "HTTP PUT", "Supports HTTP PUT requests"));
    caps.AddCapability(ProtocolCapability("http.patch", "HTTP PATCH", "Supports HTTP PATCH requests"));
    caps.AddCapability(ProtocolCapability("http.delete", "HTTP DELETE", "Supports HTTP DELETE requests"));
    caps.AddCapability(ProtocolCapability("http.keep_alive", "HTTP Keep-Alive", "Supports persistent HTTP connection reuse"));
    caps.AddCapability(ProtocolCapability("http.json", "HTTP JSON Payload", "Supports JSON payload encoding/decoding"));
    caps.AddCapability(ProtocolCapability("http.binary", "HTTP Binary Payload", "Supports octet-stream binary payloads"));
    return caps;
}

Runtime::ExecutionStepResult HTTPAdapter::Execute(
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
            "HTTPAdapter: adapter is not initialized or unavailable",
            false, false, {}, {},
            {{"adapterId", m_descriptor.adapterId}});
    }

    if (!m_transport) {
        m_stats.RecordFailure();
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::NotImplemented,
            m_descriptor.adapterId, 0, 0, -2,
            "HTTPAdapter: no IHTTPTransport attached",
            false, false, {}, {},
            {{"adapterId", m_descriptor.adapterId}});
    }

    // 1. Resolve device context
    HTTPDeviceContext device;
    {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        if (!m_devices.empty()) {
            device = m_devices.begin()->second;
        } else {
            device = HTTPDeviceContext("http://localhost", "localhost", 80);
        }
    }

    // 2. Build HTTPRequest from ExecutionStep
    HTTPRequest request = m_builder.BuildRequest(step, device.baseUrl);

    switch (request.method) {
        case HTTPMethod::GET:    m_stats.RecordGet(); break;
        case HTTPMethod::POST:   m_stats.RecordPost(); break;
        case HTTPMethod::PUT:    m_stats.RecordPut(); break;
        case HTTPMethod::DELETE: m_stats.RecordDelete(); break;
        case HTTPMethod::PATCH:  m_stats.RecordPatch(); break;
    }

    // 3. Lease ProtocolSession from IProtocolSessionManager
    std::optional<ProtocolSession> pSession;
    if (m_sessionManager) {
        pSession = m_sessionManager->AcquireSession(
            m_descriptor.adapterId, "HTTP", device.baseUrl, step.GetTimeoutMs());
    }

    HTTPSessionContext sessionCtx("esp_claw_http_session");

    // 4. Lease ConnectionHandle from IConnectionManager
    std::optional<ConnectionHandle> connectionHandle;
    if (m_connectionManager) {
        connectionHandle = m_connectionManager->AcquireConnection(
            "HTTP", device.baseUrl, step.GetTimeoutMs());
    }

    ConnectionHandle activeHandle = connectionHandle.has_value() ? connectionHandle.value() : ConnectionHandle("conn.http.default", "HTTP", device.baseUrl);

    // 5. Execute via IHTTPTransport with retry policy evaluation
    uint32_t attempt = 0;
    HTTPResponse response;

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

    m_stats.RecordBytes(response.bytesSent, response.bytesReceived);

    // 6. Parse response payload
    std::string contentType = "application/json";
    auto ctIt = response.headers.find("Content-Type");
    if (ctIt != response.headers.end()) contentType = ctIt->second;

    HTTPParsedResponse parsed = m_parser.Parse(response.payload, contentType);

    // 7. Map outcome to ExecutionStepResult
    Runtime::ExecutionStepResult stepResult = m_mapper.MapToStepResult(
        step.GetStepId(), m_descriptor.adapterId, response, parsed);

    if (stepResult.IsSuccess()) {
        m_stats.RecordSuccess();
    } else {
        m_stats.RecordFailure();
    }

    m_stats.RecordLatency(response.latencyMs);

    return stepResult;
}

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
