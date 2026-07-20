/**
 * @file WakeOnLANTransport.cpp
 * @brief Implementation of WakeOnLANTransport.
 */

#include "../../include/transports/WakeOnLANTransport.h"
#include "../../include/UdpSocket.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

namespace NetDiscovery {

// Helper to parse MAC address "XX:XX:XX:XX:XX:XX" to bytes
static std::vector<uint8_t> ParseMacAddress(const std::string& mac) {
    std::vector<uint8_t> bytes;
    std::stringstream ss(mac);
    std::string byteStr;
    while (std::getline(ss, byteStr, ':')) {
        if (!byteStr.empty()) {
            bytes.push_back(static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16)));
        }
    }
    return bytes;
}

ExecutionResult WakeOnLANTransport::Execute(const ExecutionRequest& request, const ExecutionRoute& route) {
    (void)request;
    ExecutionResult result;
    result.status = ExecutionStatus::ExecutionFailed;

    auto macIt = route.metadata.find("Target-MAC");
    if (macIt == route.metadata.end() || macIt->second.empty()) {
        result.errorMessage = "Target-MAC metadata missing from route";
        return result;
    }

    std::vector<uint8_t> macBytes = ParseMacAddress(macIt->second);
    if (macBytes.size() != 6) {
        result.errorMessage = "Invalid MAC address format";
        return result;
    }

    // Build Magic Packet
    // 6 bytes of 0xFF followed by 16 repetitions of the target MAC
    std::vector<uint8_t> magicPacket;
    for (int i = 0; i < 6; ++i) {
        magicPacket.push_back(0xFF);
    }
    for (int i = 0; i < 16; ++i) {
        magicPacket.insert(magicPacket.end(), macBytes.begin(), macBytes.end());
    }

    std::string payload(magicPacket.begin(), magicPacket.end());

    UdpSocket sock;
    if (sock.Open() != ESP_OK) {
        result.errorMessage = "WoL failed: UdpSocket::Open failed.";
        return result;
    }

    if (sock.EnableBroadcast(true) != ESP_OK) {
        result.errorMessage = "WoL failed: UdpSocket::EnableBroadcast failed.";
        sock.Close();
        return result;
    }

    // Broadcast to 255.255.255.255 on port 9 (Discard port) or 7 (Echo port)
    if (!sock.Send(payload, "255.255.255.255", 9).has_value()) {
        result.errorMessage = "WoL failed: UdpSocket::Send failed.";
        sock.Close();
        return result;
    }
    
    sock.Close();

    result.status = ExecutionStatus::Success;
    result.errorMessage = "Magic packet sent successfully.";

    return result;
}

} // namespace NetDiscovery
