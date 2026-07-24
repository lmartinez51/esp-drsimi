/**
 * @file IHTTPTransport.h
 * @brief Injectable transport interface for HTTP protocol operations (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include "protocol/http/HTTPRequest.h"
#include "protocol/http/HTTPResponse.h"
#include "protocol/http/HTTPSessionContext.h"
#include "protocol/connection/ConnectionHandle.h"

#include <functional>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Injectable transport boundary interface for HTTP wire operations.
 *
 * The HTTPAdapter interacts with wire mechanics strictly through Send().
 * Contains ZERO business logic.
 */
class IHTTPTransport {
public:
    virtual ~IHTTPTransport() = default;

    /**
     * @brief Sends an HTTPRequest using a leased ConnectionHandle.
     */
    virtual HTTPResponse Send(
        const HTTPRequest&        request,
        HTTPSessionContext&       session,
        const ConnectionHandle&   connection = ConnectionHandle()) = 0;
};

/**
 * @brief Mock transport implementation for offline unit testing without network dependencies.
 */
class MockHTTPTransport : public IHTTPTransport {
public:
    using RequestHandler = std::function<HTTPResponse(const HTTPRequest&, HTTPSessionContext&, const ConnectionHandle&)>;

    MockHTTPTransport() = default;

    explicit MockHTTPTransport(RequestHandler handler)
        : m_handler(std::move(handler)) {}

    void SetHandler(RequestHandler handler) {
        m_handler = std::move(handler);
    }

    void SetPresetResponse(HTTPResponse response) {
        m_presetResponse = std::move(response);
    }

    HTTPResponse Send(const HTTPRequest& request,
                      HTTPSessionContext& session,
                      const ConnectionHandle& connection = ConnectionHandle()) override {
        m_lastRequest    = request;
        m_lastConnection = connection;
        ++m_callCount;

        session.lastActivityTimestampMs = 1000;

        if (m_handler) {
            return m_handler(request, session, connection);
        }
        return m_presetResponse;
    }

    const HTTPRequest& GetLastRequest() const { return m_lastRequest; }
    const ConnectionHandle& GetLastConnection() const { return m_lastConnection; }
    uint32_t GetCallCount() const { return m_callCount; }

private:
    RequestHandler   m_handler;
    HTTPResponse     m_presetResponse{200, "OK", {{"Content-Type", "application/json"}}, "{\"status\":\"ok\"}", 15, 128, 256};
    HTTPRequest      m_lastRequest;
    ConnectionHandle m_lastConnection;
    uint32_t         m_callCount{0};
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
