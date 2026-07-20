/**
 * @file UdpSocket.h
 * @brief Platform-neutral UDP socket for sending and receiving datagrams.
 */

#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include "esp_err.h"

namespace NetDiscovery {

class UdpSocket {
public:
    UdpSocket() noexcept;
    ~UdpSocket() noexcept;

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    UdpSocket(UdpSocket&&) noexcept;
    UdpSocket& operator=(UdpSocket&&) noexcept;

    esp_err_t Open();
    void Close() noexcept;
    bool IsOpen() const noexcept;

    esp_err_t SetReceiveTimeout(int seconds);
    esp_err_t EnableBroadcast(bool enable = true);

    std::optional<int> Send(const std::string& data,
                            const std::string& destIp,
                            uint16_t           destPort);

    std::optional<int> Receive(char*       buffer,
                               int         bufferSize,
                               std::string& senderIp,
                               uint16_t&    senderPort);

private:
    uintptr_t m_handle;
};

} // namespace NetDiscovery
