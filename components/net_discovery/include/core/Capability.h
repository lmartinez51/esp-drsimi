/**
 * @file Capability.h
 * @brief High-level functional capability of a device.
 *
 * Capabilities represent user-visible functionality, NOT protocols.
 * Used by ESP-Claw and LLM for intention-based actions.
 */

#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief High-level functional capability of a discovered device.
 */
enum class Capability {
    Unknown,
    PowerControl,
    VolumeControl,
    Mute,
    MediaPlayback,
    MediaTransport,
    InputSelection,
    ScreenCasting,
    ApplicationLaunching,
    RemoteControl,
    Lighting,
    Brightness,
    ColorTemperature,
    ColorRGB,
    Sensor,
    Switch,
    Thermostat
};

/**
 * @brief Convert a Capability to a human-readable string.
 */
inline std::string ToString(Capability cap) {
    switch (cap) {
        case Capability::PowerControl:         return "Power Control";
        case Capability::VolumeControl:        return "Volume Control";
        case Capability::Mute:                 return "Mute";
        case Capability::MediaPlayback:        return "Media Playback";
        case Capability::MediaTransport:       return "Media Transport";
        case Capability::InputSelection:       return "Input Selection";
        case Capability::ScreenCasting:        return "Screen Casting";
        case Capability::ApplicationLaunching: return "Application Launching";
        case Capability::RemoteControl:        return "Remote Control";
        case Capability::Lighting:             return "Lighting";
        case Capability::Brightness:           return "Brightness";
        case Capability::ColorTemperature:     return "Color Temperature";
        case Capability::ColorRGB:             return "Color RGB";
        case Capability::Sensor:               return "Sensor";
        case Capability::Switch:               return "Switch";
        case Capability::Thermostat:           return "Thermostat";
        case Capability::Unknown:
        default:                               return "Unknown";
    }
}

} // namespace NetDiscovery
