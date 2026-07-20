/**
 * @file WebSocketTransport.cpp
 * @brief Implementation of WebSocket framing and handshake over TcpSocket.
 */

#include "../../include/transports/WebSocketTransport.h"
#include "../../include/HttpClient.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <random>
#include <chrono>
#include "esp_log.h"
#include <cerrno>

static const char* TAG = "NetDiscovery";

namespace NetDiscovery {

static std::string Base64Encode(const std::vector<uint8_t>& data) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int i = 0;
    uint32_t octet_a, octet_b, octet_c, triple;
    while (i < data.size()) {
        octet_a = i < data.size() ? data[i++] : 0;
        octet_b = i < data.size() ? data[i++] : 0;
        octet_c = i < data.size() ? data[i++] : 0;
        triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        encoded += encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded += encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded += i > data.size() + 1 ? '=' : encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded += i > data.size() ? '=' : encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    return encoded;
}

std::string WebSocketTransport::GenerateSecWebSocketKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::vector<uint8_t> key(16);
    for (int i = 0; i < 16; ++i) {
        key[i] = static_cast<uint8_t>(dis(gen));
    }
    return Base64Encode(key);
}

std::string WebSocketTransport::ComputeSecWebSocketAccept(const std::string& /*key*/) {
    return "";
}

bool WebSocketTransport::DoHandshake(TcpSocket& socket, const std::string& host, const std::string& path) {
    std::string key = GenerateSecWebSocketKey();
    
    std::stringstream request;
    request << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << key << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n"
            << "\r\n";
            
    std::string reqStr = request.str();
    ESP_LOGD(TAG, "[WebSocketTransport] HTTP UPGRADE REQUEST:\n%s", reqStr.c_str());

    if (!socket.Send(reqStr).has_value()) {
        ESP_LOGE(TAG, "[WebSocketTransport] Failed to send handshake request");
        return false;
    }
    
    std::string response;
    char buffer[1024];
    while (response.find("\r\n\r\n") == std::string::npos) {
        auto r = socket.Receive(buffer, sizeof(buffer));
        if (!r.has_value() || r.value() <= 0) {
            ESP_LOGE(TAG, "[WebSocketTransport] Connection closed while reading HTTP Upgrade response");
            return false;
        }
        response.append(buffer, r.value());
    }
    
    ESP_LOGD(TAG, "[WebSocketTransport] HTTP UPGRADE RESPONSE:\n%s", response.c_str());

    if (response.find("101 Switching Protocols") != std::string::npos) {
        return true;
    }
    
    ESP_LOGE(TAG, "[WebSocketTransport] HTTP Upgrade Failed! Response lacks 101 Switching Protocols");
    return false;
}

void WebSocketTransport::SendFrame(TcpSocket& socket, const std::string& payload) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    
    size_t len = payload.length();
    if (len <= 125) {
        frame.push_back(static_cast<uint8_t>(len | 0x80));
    } else if (len <= 65535) {
        frame.push_back(126 | 0x80);
        frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(len & 0xFF));
    } else {
        frame.push_back(127 | 0x80);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
        }
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    uint8_t mask[4] = {
        static_cast<uint8_t>(dis(gen)),
        static_cast<uint8_t>(dis(gen)),
        static_cast<uint8_t>(dis(gen)),
        static_cast<uint8_t>(dis(gen))
    };
    
    frame.push_back(mask[0]);
    frame.push_back(mask[1]);
    frame.push_back(mask[2]);
    frame.push_back(mask[3]);
    
    for (size_t i = 0; i < len; ++i) {
        frame.push_back(static_cast<uint8_t>(payload[i] ^ mask[i % 4]));
    }
    
    std::string frameStr(frame.begin(), frame.end());
    socket.Send(frameStr);
}

std::string WebSocketTransport::ReceiveFrame(TcpSocket& socket) {
    char header[2];
    auto r = socket.Receive(header, 2);
    if (!r.has_value() || r.value() < 2) return "";
    
    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;
    
    if (payloadLen == 126) {
        char ext[2];
        auto rx = socket.Receive(ext, 2);
        if (!rx.has_value() || rx.value() < 2) return "";
        payloadLen = (static_cast<uint8_t>(ext[0]) << 8) | static_cast<uint8_t>(ext[1]);
    } else if (payloadLen == 127) {
        char ext[8];
        auto rx = socket.Receive(ext, 8);
        if (!rx.has_value() || rx.value() < 8) return "";
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | static_cast<uint8_t>(ext[i]);
        }
    }
    
    char maskKey[4];
    if (masked) {
        auto rm = socket.Receive(maskKey, 4);
        if (!rm.has_value() || rm.value() < 4) return "";
    }
    
    std::string payload;
    if (payloadLen > 0) {
        std::vector<char> buffer(payloadLen);
        size_t totalReceived = 0;
        while (totalReceived < payloadLen) {
            auto rx = socket.Receive(buffer.data() + totalReceived, static_cast<int>(payloadLen - totalReceived));
            if (!rx.has_value() || rx.value() <= 0) break;
            totalReceived += rx.value();
        }
        
        if (masked) {
            for (size_t i = 0; i < totalReceived; ++i) {
                buffer[i] ^= maskKey[i % 4];
            }
        }
        payload.assign(buffer.data(), totalReceived);
    }
    
    if (opcode == 8) { // Close frame
        return "";
    }
    return payload;
}

ExecutionResult WebSocketTransport::Execute(const ExecutionRequest& request, const ExecutionRoute& route) {
    ExecutionResult result;
    result.status = ExecutionStatus::ExecutionFailed;
    
    auto it = route.metadata.find("WebSocket-Host");
    if (it == route.metadata.end()) {
        result.errorMessage = "Missing WebSocket-Host in metadata";
        return result;
    }
    std::string host = it->second;
    
    std::string portStr = "8001";
    it = route.metadata.find("WebSocket-Port");
    if (it != route.metadata.end()) {
        portStr = it->second;
    }
    int port = std::strtol(portStr.c_str(), nullptr, 10);
    
    std::string path = "/";
    it = route.metadata.find("WebSocket-Path");
    if (it != route.metadata.end()) {
        path = it->second;
    }

    ESP_LOGD(TAG, "[WebSocketTransport] Target: %s:%d%s", host.c_str(), port, path.c_str());

    TcpSocket socket;
    auto start = std::chrono::steady_clock::now();
    
    if (socket.Connect(host, static_cast<uint16_t>(port)) != ESP_OK) {
        ESP_LOGE(TAG, "[WebSocketTransport] Connection failed");
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "WebSocket Connection failed";
        return result;
    }
    
    if (!DoHandshake(socket, host, path)) {
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "WebSocket handshake failed";
        return result;
    }
    
    if (request.action.id != ActionId::Unknown) {
        it = route.metadata.find("WebSocket-Payload");
        if (it != route.metadata.end()) {
            std::string payload = it->second;
            SendFrame(socket, payload);
        }
    }
    
    std::string response = ReceiveFrame(socket);
    if (response.empty()) {
        ESP_LOGE(TAG, "[WebSocketTransport] Received empty WebSocket frame (connection likely closed)");
        result.status = ExecutionStatus::AuthenticationRequired;
        result.errorMessage = "Connection closed or no data";
    } else {
        ESP_LOGD(TAG, "[WebSocketTransport] FIRST WEBSOCKET FRAME:\n%s", response.c_str());
        result.status = ExecutionStatus::Success;
        result.transportDiagnostics.rawPayload = response;
    }
    
    auto end = std::chrono::steady_clock::now();
    result.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    
    return result;
}

} // namespace NetDiscovery
