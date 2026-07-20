/**
 * @file TransportFamily.h
 * @brief Strongly typed enumeration of supported execution transports.
 */

#pragma once

#include <string>

namespace NetDiscovery {

enum class TransportFamily {
    Unknown,
    SOAP,
    DIAL,
    HTTP,
    REST,
    WebSocket,
    WakeOnLAN,
    BLE,
    Matter,
    IR,
    Lua
};

inline std::string ToString(TransportFamily family) {
    switch (family) {
        case TransportFamily::SOAP:          return "SOAP";
        case TransportFamily::DIAL:          return "DIAL";
        case TransportFamily::HTTP:          return "HTTP";
        case TransportFamily::REST:          return "REST";
        case TransportFamily::WebSocket:     return "WebSocket";
        case TransportFamily::WakeOnLAN:     return "WakeOnLAN";
        case TransportFamily::BLE:           return "BLE";
        case TransportFamily::Matter:        return "Matter";
        case TransportFamily::IR:            return "IR";
        case TransportFamily::Lua:           return "Lua";
        case TransportFamily::Unknown:
        default:                             return "Unknown";
    }
}

} // namespace NetDiscovery
