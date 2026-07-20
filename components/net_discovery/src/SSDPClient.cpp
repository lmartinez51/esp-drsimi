/**
 * @file SSDPClient.cpp
 * @brief ESP-IDF implementation of SSDP discovery client.
 */

#include "../include/SSDPClient.h"
#include "../include/SSDPAnalyzer.h"
#include "../include/MulticastSocket.h"
#include "../include/PacketUtilities.h"
#include "esp_log.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <string>

static const char* TAG = "NetDiscovery";

namespace NetDiscovery {

std::atomic<bool> SSDPClient::s_stopListening{false};

namespace {
    constexpr const char* SSDP_MULTICAST_ADDR = "239.255.255.250";
    constexpr uint16_t    SSDP_PORT           = 1900;
    constexpr int         RECV_BUFFER_SIZE    = 4096;
}

SSDPClient::~SSDPClient() noexcept
{
    Shutdown();
}

esp_err_t SSDPClient::Initialize()
{
    return m_udpSocket.Open();
}

void SSDPClient::Shutdown() noexcept
{
    m_udpSocket.Close();
}

bool SSDPClient::IsInitialized() const noexcept
{
    return m_udpSocket.IsOpen();
}

std::optional<std::vector<Packet>> SSDPClient::Discover(const std::string& searchTarget,
                                                          int                mxSeconds,
                                                          int                timeoutSeconds)
{
    if (!IsInitialized()) {
        ESP_LOGE(TAG, "SSDPClient::Discover — not initialized. Call Initialize() first.");
        return std::nullopt;
    }

    if (m_udpSocket.SetReceiveTimeout(timeoutSeconds) != ESP_OK) {
        return std::nullopt;
    }

    const std::string request = SSDPAnalyzer::BuildMSearchRequest(
        SSDP_MULTICAST_ADDR, SSDP_PORT, searchTarget, mxSeconds);

    if (!m_udpSocket.Send(request, SSDP_MULTICAST_ADDR, SSDP_PORT).has_value()) {
        return std::nullopt;
    }

    std::vector<Packet> packets;
    char recvBuf[RECV_BUFFER_SIZE];

    while (true) {
        std::string senderIp;
        uint16_t    senderPort = 0;

        auto received = m_udpSocket.Receive(recvBuf, RECV_BUFFER_SIZE, senderIp, senderPort);
        if (!received.has_value()) {
            return std::nullopt;
        }
        if (received.value() == 0) {
            break;
        }

        Packet pkt;
        pkt.timestamp           = std::chrono::system_clock::now();
        pkt.transport           = TransportProtocol::UDP;
        pkt.protocol            = ProtocolType::SSDP;
        pkt.source.address      = senderIp;
        pkt.source.port         = senderPort;
        pkt.destination.address = SSDP_MULTICAST_ADDR;
        pkt.destination.port    = SSDP_PORT;
        pkt.rawPayload          = std::string(recvBuf, static_cast<std::size_t>(received.value()));
        pkt.metadata["ssdp.searchTarget"] = searchTarget;

        packets.push_back(std::move(pkt));
    }

    return packets;
}

std::optional<std::vector<DiscoveryResult>> SSDPClient::DiscoverMultiple(
    const std::vector<std::string>& targets,
    int                             mxSeconds,
    int                             timeoutSeconds)
{
    std::vector<DiscoveryResult> results;
    results.reserve(targets.size());

    for (const auto& target : targets) {
        auto packets = Discover(target, mxSeconds, timeoutSeconds);
        if (!packets.has_value()) {
            return std::nullopt;
        }
        DiscoveryResult result;
        result.searchTarget = target;
        result.packets      = std::move(packets.value());
        results.push_back(std::move(result));
    }

    return results;
}

std::optional<std::vector<DiscoveryResult>> SSDPClient::DiscoverAll(
    const std::vector<std::string>& targets,
    int                             mxSeconds,
    int                             timeoutSeconds)
{
    if (!IsInitialized()) {
        ESP_LOGE(TAG, "SSDPClient::DiscoverAll -- not initialized. Call Initialize() first.");
        return std::nullopt;
    }

    std::vector<DiscoveryResult> results;
    results.reserve(targets.size());
    for (const auto& t : targets) {
        results.push_back({t, {}});
    }

    for (const auto& target : targets) {
        const std::string request = SSDPAnalyzer::BuildMSearchRequest(
            SSDP_MULTICAST_ADDR, SSDP_PORT, target, mxSeconds);
        if (!m_udpSocket.Send(request, SSDP_MULTICAST_ADDR, SSDP_PORT).has_value()) {
            return std::nullopt;
        }
    }

    if (m_udpSocket.SetReceiveTimeout(timeoutSeconds) != ESP_OK) {
        return std::nullopt;
    }

    char recvBuf[RECV_BUFFER_SIZE];
    while (true) {
        std::string senderIp;
        uint16_t    senderPort = 0;

        auto received = m_udpSocket.Receive(recvBuf, RECV_BUFFER_SIZE, senderIp, senderPort);
        if (!received.has_value()) {
            return std::nullopt;
        }
        if (received.value() == 0) {
            break;
        }

        Packet pkt;
        pkt.timestamp           = std::chrono::system_clock::now();
        pkt.transport           = TransportProtocol::UDP;
        pkt.protocol            = ProtocolType::SSDP;
        pkt.source.address      = senderIp;
        pkt.source.port         = senderPort;
        pkt.destination.address = SSDP_MULTICAST_ADDR;
        pkt.destination.port    = SSDP_PORT;
        pkt.rawPayload          = std::string(recvBuf, static_cast<std::size_t>(received.value()));

        using PacketUtilities = NetDiscovery::PacketUtilities;
        const std::string st = PacketUtilities::ExtractHeaderValue(pkt.rawPayload, "ST");

        bool placed = false;
        for (auto& result : results) {
            if (result.searchTarget == st || result.searchTarget == "ssdp:all") {
                if (result.searchTarget == st) {
                    pkt.metadata["ssdp.searchTarget"] = st;
                    result.packets.push_back(pkt);
                    placed = true;
                    break;
                }
            }
        }
        if (!placed) {
            for (auto& result : results) {
                if (result.searchTarget == "ssdp:all") {
                    pkt.metadata["ssdp.searchTarget"] = "ssdp:all";
                    result.packets.push_back(pkt);
                    break;
                }
            }
        }
    }

    return results;
}

esp_err_t SSDPClient::ListenPassive(std::function<void(const Packet&)> onPacket, int durationSeconds)
{
    MulticastSocket mcast;
    if (mcast.Open(SSDP_PORT) != ESP_OK) return ESP_FAIL;
    if (mcast.JoinGroup(SSDP_MULTICAST_ADDR) != ESP_OK) return ESP_FAIL;
    if (mcast.SetReceiveTimeout(1) != ESP_OK) return ESP_FAIL;

    s_stopListening = false;

    const auto startTime = std::chrono::steady_clock::now();
    char recvBuf[RECV_BUFFER_SIZE];

    ESP_LOGI(TAG, "Passive listening on %s:%d", SSDP_MULTICAST_ADDR, SSDP_PORT);

    while (!s_stopListening) {
        if (durationSeconds > 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            if (elapsed >= static_cast<long long>(durationSeconds)) {
                break;
            }
        }

        std::string senderIp;
        uint16_t    senderPort = 0;
        auto received = mcast.Receive(recvBuf, RECV_BUFFER_SIZE, senderIp, senderPort);

        if (!received.has_value()) {
            return ESP_FAIL;
        }
        if (received.value() == 0) continue;

        Packet pkt;
        pkt.timestamp           = std::chrono::system_clock::now();
        pkt.transport           = TransportProtocol::Multicast;
        pkt.protocol            = ProtocolType::SSDP;
        pkt.source.address      = senderIp;
        pkt.source.port         = senderPort;
        pkt.destination.address = SSDP_MULTICAST_ADDR;
        pkt.destination.port    = SSDP_PORT;
        pkt.rawPayload          = std::string(recvBuf, static_cast<std::size_t>(received.value()));

        if (onPacket) {
            onPacket(pkt);
        }
    }

    mcast.Close();
    return ESP_OK;
}

} // namespace NetDiscovery
