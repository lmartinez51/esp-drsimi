/**
 * @file BindingPriority.h
 * @brief Priority classification and evaluation model for protocol action bindings (v5.0.0 Architecture Phase 8).
 * 
 * Defines standard numerical priority ratings for candidate bindings across protocols.
 * Higher values represent higher default selection preference.
 */

#pragma once

#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Standard protocol priority constants.
 */
struct PriorityLevel {
    static constexpr int Matter  = 100; // Native smart home fabric standard
    static constexpr int UPnP    = 90;  // High-reliability local network IP discovery/control
    static constexpr int HTTP    = 80;  // RESTful / Web API endpoint
    static constexpr int BLE     = 70;  // Direct low-energy radio transport
    static constexpr int MQTT    = 60;  // Asynchronous message bus
    static constexpr int Zigbee  = 50;  // IEEE 802.15.4 mesh network
    static constexpr int Thread  = 45;  // IP-based mesh network
    static constexpr int Default = 50;  // Baseline unclassified protocol rating
    static constexpr int IR      = 20;  // One-way unacknowledged optical emitter
};

/**
 * @brief Resolves default priority score based on protocol name string.
 */
inline int GetDefaultPriorityForProtocol(const std::string& protocol) {
    if (protocol == "Matter" || protocol == "matter") return PriorityLevel::Matter;
    if (protocol == "UPnP" || protocol == "upnp" || protocol == "SOAP") return PriorityLevel::UPnP;
    if (protocol == "HTTP" || protocol == "http" || protocol == "REST") return PriorityLevel::HTTP;
    if (protocol == "BLE" || protocol == "ble" || protocol == "Bluetooth") return PriorityLevel::BLE;
    if (protocol == "MQTT" || protocol == "mqtt") return PriorityLevel::MQTT;
    if (protocol == "Zigbee" || protocol == "zigbee") return PriorityLevel::Zigbee;
    if (protocol == "Thread" || protocol == "thread") return PriorityLevel::Thread;
    if (protocol == "IR" || protocol == "ir" || protocol == "Infrared") return PriorityLevel::IR;
    return PriorityLevel::Default;
}

} // namespace Binding
} // namespace NetDiscovery
