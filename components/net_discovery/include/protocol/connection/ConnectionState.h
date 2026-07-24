/**
 * @file ConnectionState.h
 * @brief Connection status codes and state models (v5.0.0 Architecture Phase 11.2).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Runtime operational status of a transport connection.
 */
enum class ConnectionStatus {
    Connected,
    Connecting,
    Reconnecting,
    Closing,
    Closed,
    Failed,
    AuthenticationExpired
};

/**
 * @brief Converts ConnectionStatus enum to string.
 */
inline std::string ToString(ConnectionStatus status) {
    switch (status) {
        case ConnectionStatus::Connected:             return "Connected";
        case ConnectionStatus::Connecting:            return "Connecting";
        case ConnectionStatus::Reconnecting:          return "Reconnecting";
        case ConnectionStatus::Closing:               return "Closing";
        case ConnectionStatus::Closed:                return "Closed";
        case ConnectionStatus::Failed:                return "Failed";
        case ConnectionStatus::AuthenticationExpired: return "AuthenticationExpired";
        default:                                      return "Unknown";
    }
}

} // namespace Protocol
} // namespace NetDiscovery
