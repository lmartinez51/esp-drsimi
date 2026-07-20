/**
 * @file DeviceClass.h
 * @brief Enums for classifying devices and their roles.
 */

#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief The primary identity/form factor of the device.
 */
enum class PrimaryDeviceClass {
    InternetGateway,
    SmartTV,
    StreamingDevice,
    MediaRenderer,
    MediaServer,
    IoTDevice,
    Speaker,
    Unknown
};

/**
 * @brief The capabilities or roles the device fulfills.
 */
enum class DeviceRole {
    MediaRenderer,
    MediaServer,
    DIALReceiver,
    RemoteControlTarget,
    Display,
    NetworkRouter,
    Unknown
};

inline std::string ToString(PrimaryDeviceClass cls) {
    switch (cls) {
        case PrimaryDeviceClass::InternetGateway: return "Internet Gateway";
        case PrimaryDeviceClass::SmartTV:         return "Smart TV";
        case PrimaryDeviceClass::StreamingDevice: return "Streaming Device";
        case PrimaryDeviceClass::MediaRenderer:   return "Media Renderer";
        case PrimaryDeviceClass::MediaServer:     return "Media Server";
        case PrimaryDeviceClass::IoTDevice:       return "IoT Device";
        case PrimaryDeviceClass::Speaker:         return "Speaker";
        default:                                  return "Unknown Class";
    }
}

inline std::string ToString(DeviceRole role) {
    switch (role) {
        case DeviceRole::MediaRenderer:       return "Media Renderer Role";
        case DeviceRole::MediaServer:         return "Media Server Role";
        case DeviceRole::DIALReceiver:        return "DIAL Receiver";
        case DeviceRole::RemoteControlTarget: return "Remote Control Target";
        case DeviceRole::Display:             return "Display";
        case DeviceRole::NetworkRouter:       return "Network Router";
        default:                              return "Unknown Role";
    }
}

} // namespace NetDiscovery
