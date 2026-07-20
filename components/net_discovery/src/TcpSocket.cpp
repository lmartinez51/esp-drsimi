/**
 * @file TcpSocket.cpp
 * @brief ESP-IDF lwIP TCP socket.
 */

#include "../include/TcpSocket.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include <iostream>
#include <cstring>

static const char* TAG = "NetDiscovery";

namespace NetDiscovery {

TcpSocket::TcpSocket() noexcept : m_handle(0) {}

TcpSocket::~TcpSocket() noexcept { Disconnect(); }

TcpSocket::TcpSocket(TcpSocket&& other) noexcept : m_handle(other.m_handle)
{
    other.m_handle = 0;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept
{
    if (this != &other) {
        Disconnect();
        m_handle = other.m_handle;
        other.m_handle = 0;
    }
    return *this;
}

esp_err_t TcpSocket::Connect(const std::string& host, uint16_t port)
{
    if (IsConnected()) {
        Disconnect();
    }

    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    std::string portStr = std::to_string(port);

    int r = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (r != 0) {
        ESP_LOGE(TAG, "TcpSocket::Connect — getaddrinfo failed for %s", host.c_str());
        return ESP_FAIL;
    }

    int sock = -1;
    for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock == -1) {
            continue;
        }

        struct timeval tv;
        tv.tv_sec = 15;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        if (connect(sock, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            close(sock);
            sock = -1;
            continue;
        }
        
        break; // Connected successfully
    }

    freeaddrinfo(result);

    if (sock == -1) {
        ESP_LOGE(TAG, "TcpSocket::Connect — unable to connect to %s:%d", host.c_str(), port);
        return ESP_FAIL;
    }

    m_handle = static_cast<uintptr_t>(sock);
    return ESP_OK;
}

void TcpSocket::Disconnect() noexcept
{
    if (m_handle != 0) {
        int sock = static_cast<int>(m_handle);
        shutdown(sock, SHUT_RDWR);
        close(sock);
        m_handle = 0;
    }
}

bool TcpSocket::IsConnected() const noexcept
{
    return m_handle != 0;
}

std::optional<int> TcpSocket::Send(const std::string& data)
{
    if (!IsConnected()) {
        ESP_LOGE(TAG, "TcpSocket::Send — Not connected.");
        return std::nullopt;
    }

    int sock = static_cast<int>(m_handle);
    int totalSent = 0;
    int dataLen = static_cast<int>(data.length());
    const char* ptr = data.c_str();

    while (totalSent < dataLen) {
        int r = send(sock, ptr + totalSent, dataLen - totalSent, 0);
        if (r == -1) {
            ESP_LOGE(TAG, "TcpSocket::Send failed: %s", strerror(errno));
            return std::nullopt;
        }
        totalSent += r;
    }

    return totalSent;
}

std::optional<int> TcpSocket::Receive(char* buffer, int bufferSize)
{
    if (!IsConnected()) {
        ESP_LOGE(TAG, "TcpSocket::Receive — Not connected.");
        return std::nullopt;
    }
    
    if (bufferSize <= 0) return 0;

    int sock = static_cast<int>(m_handle);
    int r = recv(sock, buffer, bufferSize, 0);
    
    if (r == -1) {
        int err = errno;
        if (err == EAGAIN || err == EWOULDBLOCK) {
            return 0; // Timeout is not an exception here
        }
        ESP_LOGE(TAG, "TcpSocket::Receive failed: %s", strerror(err));
        return std::nullopt;
    }
    
    return r;
}

} // namespace NetDiscovery
