/**
 * @file MulticastSocket.cpp
 * @brief ESP-IDF lwIP implementation of MulticastSocket.
 */

#include "../include/MulticastSocket.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include <cstring>
#include <iostream>

static const char* TAG = "NetDiscovery";

namespace NetDiscovery {

MulticastSocket::MulticastSocket() noexcept
    : m_handle(static_cast<uintptr_t>(-1))
    , m_groupJoined(false)
{}

MulticastSocket::~MulticastSocket() noexcept
{
    Close();
}

MulticastSocket::MulticastSocket(MulticastSocket&& other) noexcept
    : m_handle(other.m_handle)
    , m_joinedGroup(std::move(other.m_joinedGroup))
    , m_groupJoined(other.m_groupJoined)
{
    other.m_handle      = static_cast<uintptr_t>(-1);
    other.m_groupJoined = false;
}

MulticastSocket& MulticastSocket::operator=(MulticastSocket&& other) noexcept
{
    if (this != &other) {
        Close();
        m_handle      = other.m_handle;
        m_joinedGroup = std::move(other.m_joinedGroup);
        m_groupJoined = other.m_groupJoined;
        other.m_handle      = static_cast<uintptr_t>(-1);
        other.m_groupJoined = false;
    }
    return *this;
}

esp_err_t MulticastSocket::Open(uint16_t port)
{
    if (IsOpen()) return ESP_OK;

    const int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        ESP_LOGE(TAG, "MulticastSocket::Open — socket() failed: %s", strerror(errno));
        return ESP_FAIL;
    }

    const int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        ESP_LOGE(TAG, "MulticastSocket::Open — setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        close(sock);
        return ESP_FAIL;
    }

    sockaddr_in bindAddr{};
    bindAddr.sin_family      = AF_INET;
    bindAddr.sin_port        = htons(port);
    bindAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<const sockaddr*>(&bindAddr), sizeof(bindAddr)) == -1) {
        ESP_LOGE(TAG, "MulticastSocket::Open — bind() to port %d failed: %s", port, strerror(errno));
        close(sock);
        return ESP_FAIL;
    }

    m_handle = static_cast<uintptr_t>(sock);
    ESP_LOGD(TAG, "[MulticastSocket] Opened, bound to port %d.", port);
    return ESP_OK;
}

void MulticastSocket::Close() noexcept
{
    if (!IsOpen()) return;

    LeaveGroup();

    close(static_cast<int>(m_handle));
    m_handle = static_cast<uintptr_t>(-1);
    ESP_LOGD(TAG, "[MulticastSocket] Closed.");
}

bool MulticastSocket::IsOpen() const noexcept
{
    return m_handle != static_cast<uintptr_t>(-1);
}

esp_err_t MulticastSocket::JoinGroup(const std::string& multicastAddr)
{
    if (!IsOpen()) {
        ESP_LOGE(TAG, "MulticastSocket::JoinGroup — Socket closed");
        return ESP_FAIL;
    }

    ip_mreq mreq{};
    inet_pton(AF_INET, multicastAddr.c_str(), &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;

    if (setsockopt(static_cast<int>(m_handle), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
        ESP_LOGE(TAG, "MulticastSocket::JoinGroup — IP_ADD_MEMBERSHIP failed: %s", strerror(errno));
        return ESP_FAIL;
    }

    m_joinedGroup = multicastAddr;
    m_groupJoined = true;
    ESP_LOGD(TAG, "[MulticastSocket] Joined group %s.", multicastAddr.c_str());
    return ESP_OK;
}

void MulticastSocket::LeaveGroup() noexcept
{
    if (!m_groupJoined || !IsOpen()) return;

    ip_mreq mreq{};
    inet_pton(AF_INET, m_joinedGroup.c_str(), &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;

    setsockopt(static_cast<int>(m_handle), IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));

    m_groupJoined = false;
    ESP_LOGD(TAG, "[MulticastSocket] Left group %s.", m_joinedGroup.c_str());
}

esp_err_t MulticastSocket::SetReceiveTimeout(int seconds)
{
    if (!IsOpen()) {
        ESP_LOGE(TAG, "MulticastSocket::SetReceiveTimeout — Socket closed");
        return ESP_FAIL;
    }

    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    if (setsockopt(static_cast<int>(m_handle), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        ESP_LOGE(TAG, "MulticastSocket::SetReceiveTimeout failed: %s", strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}

std::optional<int> MulticastSocket::Receive(char*        buffer,
                                            int          bufferSize,
                                            std::string& senderIp,
                                            uint16_t&    senderPort)
{
    if (!IsOpen()) {
        ESP_LOGE(TAG, "MulticastSocket::Receive — Socket closed");
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
            return 0;
        }
        ESP_LOGE(TAG, "MulticastSocket::Receive — recvfrom() failed: %s", strerror(err));
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
