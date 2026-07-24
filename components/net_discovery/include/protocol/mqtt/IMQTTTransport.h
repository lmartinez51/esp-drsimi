/**
 * @file IMQTTTransport.h
 * @brief Injectable transport interface for MQTT protocol operations (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include "protocol/mqtt/MQTTRequest.h"
#include "protocol/mqtt/MQTTResponse.h"
#include "protocol/mqtt/MQTTSessionContext.h"
#include "protocol/connection/ConnectionHandle.h"

#include <functional>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Injectable transport boundary interface for MQTT operations.
 *
 * The MQTTAdapter interacts with network mechanics strictly through Send().
 * The transport implementation handles socket mechanics, packet serialization, and wire protocol.
 */
class IMQTTTransport {
public:
    virtual ~IMQTTTransport() = default;

    /**
     * @brief Sends an MQTTRequest using a leased ConnectionHandle and mutates session context.
     */
    virtual MQTTResponse Send(
        const MQTTRequest&        request,
        MQTTSessionContext&       session,
        const ConnectionHandle&   connection = ConnectionHandle()) = 0;
};

/**
 * @brief Mock transport implementation for offline unit testing without network dependencies.
 */
class MockMQTTTransport : public IMQTTTransport {
public:
    using RequestHandler = std::function<MQTTResponse(const MQTTRequest&, MQTTSessionContext&, const ConnectionHandle&)>;

    MockMQTTTransport() = default;

    explicit MockMQTTTransport(RequestHandler handler)
        : m_handler(std::move(handler)) {}

    void SetHandler(RequestHandler handler) {
        m_handler = std::move(handler);
    }

    void SetPresetResponse(MQTTResponse response) {
        m_presetResponse = std::move(response);
    }

    MQTTResponse Send(const MQTTRequest& request,
                      MQTTSessionContext& session,
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

    const MQTTRequest& GetLastRequest() const { return m_lastRequest; }
    const ConnectionHandle& GetLastConnection() const { return m_lastConnection; }
    uint32_t GetCallCount() const { return m_callCount; }

private:
    RequestHandler   m_handler;
    MQTTResponse     m_presetResponse{0, "OK", 1, 0, 10, 64, 32};
    MQTTRequest      m_lastRequest;
    ConnectionHandle m_lastConnection;
    uint32_t         m_callCount{0};
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
