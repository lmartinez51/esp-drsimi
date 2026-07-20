/**
 * @file UdpSocket.cpp
 * @brief ESP-IDF lwIP implementation of UdpSocket.
 */

#include "../include/UdpSocket.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include <chrono>
#include <cstring>
#include <iostream>

static const char* TAG = "NetDiscovery";

namespace NetDiscovery {

UdpSocket::UdpSocket() noexcept
    : m_handle(static_cast<uintptr_t>(-1))
{}

UdpSocket::~UdpSocket() noexcept
{
    Close();
}

UdpSocket::UdpSocket(UdpSocket&& other) noexcept
    : m_handle(other.m_handle)
{
    other.m_handle = static_cast<uintptr_t>(-1);
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept
{
    if (this != &other) {
        Close();
        m_handle = other.m_handle;
        other.m_handle = static_cast<uintptr_t>(-1);
    }
    return *this;
}

esp_err_t UdpSocket::Open()
{
    if (IsOpen()) return ESP_OK;

    const int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        const int err = errno;
        ESP_LOGE(TAG, "UdpSocket::Open — socket() failed: %s", strerror(err));
        return ESP_FAIL;
    }

    m_handle = static_cast<uintptr_t>(sock);
    ESP_LOGD(TAG, "[UdpSocket] Opened (descriptor=%d).", sock);
    return ESP_OK;
}

void UdpSocket::Close() noexcept
{
    if (!IsOpen()) return;

    const int sock = static_cast<int>(m_handle);
    close(sock);
    m_handle = static_cast<uintptr_t>(-1);
    ESP_LOGD(TAG, "[UdpSocket] Closed.");
}

bool UdpSocket::IsOpen() const noexcept
{
    return m_handle != static_cast<uintptr_t>(-1);
}

esp_err_t UdpSocket::SetReceiveTimeout(int seconds)
{
    if (!IsOpen()) {
        ESP_LOGE(TAG, "Cannot set timeout on a closed socket");
        return ESP_FAIL;
    }
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    if (setsockopt(static_cast<int>(m_handle), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        ESP_LOGE(TAG, "UdpSocket::SetReceiveTimeout failed: %s", strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t UdpSocket::EnableBroadcast(bool enable)
{
    if (!IsOpen()) {
        ESP_LOGE(TAG, "Cannot set broadcast on a closed socket");
        return ESP_FAIL;
    }
    
    int bBroadcast = enable ? 1 : 0;
    if (setsockopt(static_cast<int>(m_handle), SOL_SOCKET, SO_BROADCAST,
                   &bBroadcast, sizeof(bBroadcast)) == -1) {
        ESP_LOGE(TAG, "setsockopt(SO_BROADCAST) failed: %s", strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}

std::optional<int> UdpSocket::Send(const std::string& data,
                                   const std::string& destIp,
                                   uint16_t           destPort)
{
    if (!IsOpen()) {
        ESP_LOGE(TAG, "UdpSocket::Send — Socket closed");
        return std::nullopt;
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(destPort);
    inet_pton(AF_INET, destIp.c_str(), &dest.sin_addr);

    const int sent = sendto(
        static_cast<int>(m_handle),
        data.c_str(),
        static_cast<int>(data.size()),
        0,
        reinterpret_cast<const sockaddr*>(&dest),
        sizeof(dest)
    );

    if (sent == -1) {
        ESP_LOGE(TAG, "UdpSocket::Send — sendto() failed: %s", strerror(errno));
        return std::nullopt;
    }

    return sent;
}

std::optional<int> UdpSocket::Receive(char*        buffer,
                                      int          bufferSize,
                                      std::string& senderIp,
                                      uint16_t&    senderPort)
{
    if (!IsOpen()) {
        ESP_LOGE(TAG, "UdpSocket::Receive — Socket closed");
        return std::nullopt;
    }

    sockaddr_in senderAddr{};
    socklen_t   addrLen = sizeof(senderAddr);

    std::memset(buffer, 0, static_cast<std::size_t>(bufferSize));

    const int received = recvfrom(
        static_cast<int>(m_handle),
        buffer,
        bufferSize - 1,
        0,
        reinterpret_cast<sockaddr*>(&senderAddr),
        &addrLen
    );

    if (received == -1) {
        const int err = errno;
        if (err == EAGAIN || err == EWOULDBLOCK) {
            return 0;  // Timeout sentinel — not an error.
        }
        ESP_LOGE(TAG, "UdpSocket::Receive — recvfrom() failed: %s", strerror(err));
        return std::nullopt;
    }

    buffer[received] = '\0';
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(senderAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    senderIp = ipStr;
    senderPort = ntohs(senderAddr.sin_port);
    return received;
}

} // namespace NetDiscovery
