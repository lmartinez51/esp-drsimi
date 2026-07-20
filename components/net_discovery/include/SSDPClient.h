/**
 * @file SSDPClient.h
 * @brief SSDP discovery client — transport layer only.
 */

#pragma once

#include "UdpSocket.h"
#include "core/Packet.h"
#include "esp_err.h"

#include <atomic>
#include <functional>
#include <string>
#include <vector>
#include <optional>

namespace NetDiscovery
{
    struct DiscoveryResult
    {
        std::string searchTarget;
        std::vector<Packet> packets;
    };

    class SSDPClient
    {
    public:
        SSDPClient() = default;
        ~SSDPClient() noexcept;

        SSDPClient(const SSDPClient &) = delete;
        SSDPClient &operator=(const SSDPClient &) = delete;
        SSDPClient(SSDPClient &&) noexcept = default;
        SSDPClient &operator=(SSDPClient &&) noexcept = default;

        esp_err_t Initialize();
        void Shutdown() noexcept;
        bool IsInitialized() const noexcept;

        std::optional<std::vector<Packet>> Discover(const std::string &searchTarget,
                                                    int mxSeconds,
                                                    int timeoutSeconds);

        std::optional<std::vector<DiscoveryResult>> DiscoverMultiple(
            const std::vector<std::string> &targets,
            int mxSeconds,
            int timeoutSeconds);

        std::optional<std::vector<DiscoveryResult>> DiscoverAll(
            const std::vector<std::string> &targets,
            int mxSeconds,
            int timeoutSeconds);

        esp_err_t ListenPassive(std::function<void(const Packet &)> onPacket,
                                int durationSeconds = 0);

        static std::atomic<bool> s_stopListening;

    private:
        UdpSocket m_udpSocket;
    };

} // namespace NetDiscovery
