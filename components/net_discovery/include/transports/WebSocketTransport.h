/**
 * @file WebSocketTransport.h
 * @brief Generic WebSocket transport using raw TcpSocket.
 */

#pragma once

#include "../ICommunicationTransport.h"
#include "../TcpSocket.h"
#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Executes requests over an unencrypted WebSocket (ws://) connection.
 * 
 * Implements RFC 6455 framing and handshake manually over a raw TcpSocket 
 * to avoid external dependencies.
 */
class WebSocketTransport : public ICommunicationTransport {
public:
    TransportFamily GetFamily() const override {
        return TransportFamily::WebSocket;
    }

    ExecutionResult Execute(const ExecutionRequest& request, const ExecutionRoute& route) override;

private:
    bool DoHandshake(TcpSocket& socket, const std::string& host, const std::string& path);
    void SendFrame(TcpSocket& socket, const std::string& payload);
    std::string ReceiveFrame(TcpSocket& socket);
    std::string GenerateSecWebSocketKey();
    std::string ComputeSecWebSocketAccept(const std::string& key);
};

} // namespace NetDiscovery
