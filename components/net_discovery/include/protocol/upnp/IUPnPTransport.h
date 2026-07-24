/**
 * @file IUPnPTransport.h
 * @brief Frozen transport contract interface consuming ConnectionHandle (v5.0.0 Architecture Phase 11.2).
 */

#pragma once

#include "protocol/upnp/UPnPRequest.h"
#include "protocol/upnp/UPnPResponse.h"
#include "protocol/upnp/UPnPSessionContext.h"
#include "protocol/connection/ConnectionHandle.h"

#include <functional>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Permanent, frozen transport boundary interface.
 *
 * Transport consumes a leased ConnectionHandle. Connection acquisition and release
 * take place outside transport invocation.
 */
class IUPnPTransport {
public:
    virtual ~IUPnPTransport() = default;

    /**
     * @brief Sends a UPnPRequest using a leased ConnectionHandle.
     */
    virtual UPnPResponse Send(
        const UPnPRequest&         request,
        UPnPSessionContext&       session,
        const ConnectionHandle&   connection = ConnectionHandle()) = 0;
};

/**
 * @brief Mock transport implementation for unit testing without network dependencies.
 */
class MockUPnPTransport : public IUPnPTransport {
public:
    using RequestHandler = std::function<UPnPResponse(const UPnPRequest&, UPnPSessionContext&, const ConnectionHandle&)>;

    MockUPnPTransport() = default;

    explicit MockUPnPTransport(RequestHandler handler)
        : m_handler(std::move(handler)) {}

    void SetHandler(RequestHandler handler) {
        m_handler = std::move(handler);
    }

    void SetPresetResponse(UPnPResponse response) {
        m_presetResponse = std::move(response);
    }

    UPnPResponse Send(const UPnPRequest& request,
                      UPnPSessionContext& session,
                      const ConnectionHandle& connection = ConnectionHandle()) override {
        m_lastRequest    = request;
        m_lastConnection = connection;
        ++m_callCount;

        session.NextSequence();
        session.lastLatencyMs = 15;

        if (m_handler) {
            return m_handler(request, session, connection);
        }
        return m_presetResponse;
    }

    const UPnPRequest& GetLastRequest() const { return m_lastRequest; }
    const ConnectionHandle& GetLastConnection() const { return m_lastConnection; }
    uint32_t GetCallCount() const { return m_callCount; }

private:
    RequestHandler   m_handler;
    UPnPResponse     m_presetResponse{200, "OK", "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:MockResponse xmlns:u=\"urn:schemas-upnp-org:service:Mock:1\"><Result>OK</Result></u:MockResponse></s:Body></s:Envelope>", 15, {}, {}};
    UPnPRequest      m_lastRequest;
    ConnectionHandle m_lastConnection;
    uint32_t         m_callCount{0};
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
