/**
 * @file TcpSocket.h
 * @brief Platform-neutral TCP socket.
 */

#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include "esp_err.h"

namespace NetDiscovery {

class TcpSocket {
public:
    TcpSocket() noexcept;
    ~TcpSocket() noexcept;

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&&) noexcept;
    TcpSocket& operator=(TcpSocket&&) noexcept;

    esp_err_t Connect(const std::string& host, uint16_t port);
    void Disconnect() noexcept;
    bool IsConnected() const noexcept;

    std::optional<int> Send(const std::string& data);
    std::optional<int> Receive(char* buffer, int bufferSize);

private:
    uintptr_t m_handle;
};

} // namespace NetDiscovery
