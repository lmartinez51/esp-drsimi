/**
 * @file NetworkFingerprint.h
 * @brief Represents the derived canonical identity of a network environment.
 */

#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief Accumulated evidence about a network environment.
 */
struct NetworkEvidence {
    std::string ssid;
    std::string gatewayMac;
    std::string gatewayIp;
    std::string subnet;
    std::string dnsServer;
    // Reserved for future ESP-IDF specific identifiers
};

/**
 * @brief Derived stable identity of a network environment.
 */
class NetworkFingerprint {
public:
    NetworkFingerprint() = default;

    /**
     * @brief Computes a stable, canonical identifier from the available evidence.
     * @return Deterministic string hash of the evidence.
     */
    std::string CalculateId() const {
        // Basic heuristic for Phase 5.5: rely heavily on gateway MAC or IP.
        // In the future this can do an actual hash of multiple available properties.
        if (!evidence.gatewayMac.empty()) {
            return "net_mac_" + evidence.gatewayMac;
        }
        if (!evidence.gatewayIp.empty()) {
            return "net_ip_" + evidence.gatewayIp;
        }
        if (!evidence.ssid.empty()) {
            return "net_ssid_" + evidence.ssid;
        }
        return "net_unknown";
    }
    
    NetworkEvidence evidence;
    long long createdTimestamp{0};
    long long lastSeenTimestamp{0};
};

} // namespace NetDiscovery
