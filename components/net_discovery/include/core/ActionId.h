#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief Strongly typed identifier for all known framework actions.
 */
enum class ActionId {
    Unknown,
    PowerOn, 
    PowerOff,
    VolumeUp, 
    VolumeDown, 
    SetVolume, 
    GetVolume,
    Mute, 
    Unmute,
    LaunchApplication,
    Play, 
    Pause, 
    Stop, 
    Next, 
    Previous,
    Seek,
    SelectInput,
    SendKey
};

inline std::string ToString(ActionId id) {
    switch (id) {
        case ActionId::PowerOn: return "PowerOn";
        case ActionId::PowerOff: return "PowerOff";
        case ActionId::VolumeUp: return "VolumeUp";
        case ActionId::VolumeDown: return "VolumeDown";
        case ActionId::SetVolume: return "SetVolume";
        case ActionId::GetVolume: return "GetVolume";
        case ActionId::Mute: return "Mute";
        case ActionId::Unmute: return "Unmute";
        case ActionId::LaunchApplication: return "LaunchApplication";
        case ActionId::Play: return "Play";
        case ActionId::Pause: return "Pause";
        case ActionId::Stop: return "Stop";
        case ActionId::Next: return "Next";
        case ActionId::Previous: return "Previous";
        case ActionId::Seek: return "Seek";
        case ActionId::SelectInput: return "SelectInput";
        case ActionId::SendKey: return "SendKey";
        default: return "Unknown";
    }
}

inline ActionId FromString(const std::string& str) {
    if (str == "PowerOn") return ActionId::PowerOn;
    if (str == "PowerOff") return ActionId::PowerOff;
    if (str == "VolumeUp") return ActionId::VolumeUp;
    if (str == "VolumeDown") return ActionId::VolumeDown;
    if (str == "SetVolume") return ActionId::SetVolume;
    if (str == "GetVolume") return ActionId::GetVolume;
    if (str == "Mute") return ActionId::Mute;
    if (str == "Unmute") return ActionId::Unmute;
    if (str == "LaunchApplication") return ActionId::LaunchApplication;
    if (str == "Play") return ActionId::Play;
    if (str == "Pause") return ActionId::Pause;
    if (str == "Stop") return ActionId::Stop;
    if (str == "Next") return ActionId::Next;
    if (str == "Previous") return ActionId::Previous;
    if (str == "Seek") return ActionId::Seek;
    if (str == "SelectInput") return ActionId::SelectInput;
    if (str == "SendKey") return ActionId::SendKey;
    // Handle parameterized forms from legacy ActionResolver
    if (str == "SetVolume(level)") return ActionId::SetVolume;
    if (str == "LaunchApplication(name)") return ActionId::LaunchApplication;
    if (str == "SelectInput(input)") return ActionId::SelectInput;
    if (str == "SendKey(key)") return ActionId::SendKey;
    
    return ActionId::Unknown;
}

} // namespace NetDiscovery
