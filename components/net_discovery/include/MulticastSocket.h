/**
 * @file MulticastSocket.h
 * @brief Platform-neutral UDP multicast socket for passive reception.
 */

#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include "esp_err.h"

namespace NetDiscovery {

class MulticastSocket {
public:
    MulticastSocket() noexcept;
    ~MulticastSocket() noexcept;

    MulticastSocket(const MulticastSocket&) = delete;
    MulticastSocket& operator=(const MulticastSocket&) = delete;

    MulticastSocket(MulticastSocket&&) noexcept;
    MulticastSocket& operator=(MulticastSocket&&) noexcept;

    esp_err_t Open(uint16_t port);
    void Close() noexcept;
    bool IsOpen() const noexcept;

    esp_err_t JoinGroup(const std::string& multicastAddr);
    void LeaveGroup() noexcept;

    esp_err_t SetReceiveTimeout(int seconds);

    std::optional<int> Receive(char*       buffer,
                               int         bufferSize,
                               std::string& senderIp,
                               uint16_t&    senderPort);

private:
    uintptr_t  m_handle;
    std::string m_joinedGroup;
    bool        m_groupJoined;
};

} // namespace NetDiscovery
