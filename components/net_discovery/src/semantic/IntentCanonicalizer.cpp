#include "semantic/IntentCanonicalizer.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace semantic {

static std::string ToLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return result;
}

NetDiscovery::ActionId IntentCanonicalizer::Normalize(const std::string& rawIntent) const {
    std::string lowerIntent = ToLower(rawIntent);

    static const std::unordered_map<std::string, NetDiscovery::ActionId> s_aliasMap = {
        // Power Controls
        {"power_on",           NetDiscovery::ActionId::PowerOn},
        {"turn_on",            NetDiscovery::ActionId::PowerOn},
        {"on",                 NetDiscovery::ActionId::PowerOn},

        {"power_off",          NetDiscovery::ActionId::PowerOff},
        {"turn_off",           NetDiscovery::ActionId::PowerOff},
        {"off",                NetDiscovery::ActionId::PowerOff},

        // Volume Controls
        {"volume_up",          NetDiscovery::ActionId::VolumeUp},
        {"louder",             NetDiscovery::ActionId::VolumeUp},
        {"vol_up",             NetDiscovery::ActionId::VolumeUp},

        {"volume_down",        NetDiscovery::ActionId::VolumeDown},
        {"quieter",            NetDiscovery::ActionId::VolumeDown},
        {"vol_down",           NetDiscovery::ActionId::VolumeDown},

        {"set_volume",         NetDiscovery::ActionId::SetVolume},
        {"volume",             NetDiscovery::ActionId::SetVolume},
        {"get_volume",         NetDiscovery::ActionId::GetVolume},
        {"read_volume",        NetDiscovery::ActionId::GetVolume},

        // Mute / Audio Controls
        {"mute",               NetDiscovery::ActionId::Mute},
        {"silence",            NetDiscovery::ActionId::Mute},
        {"activate_mute",      NetDiscovery::ActionId::Mute},

        {"unmute",             NetDiscovery::ActionId::Unmute},
        {"restore_audio",      NetDiscovery::ActionId::Unmute},

        // Application Launching
        {"launch_app",         NetDiscovery::ActionId::LaunchApplication},
        {"launch_application", NetDiscovery::ActionId::LaunchApplication},
        {"open_app",           NetDiscovery::ActionId::LaunchApplication},
        {"watch_netflix",      NetDiscovery::ActionId::LaunchApplication},
        {"open_netflix",       NetDiscovery::ActionId::LaunchApplication},

        // Media Playback Controls
        {"play",               NetDiscovery::ActionId::Play},
        {"resume",             NetDiscovery::ActionId::Play},

        {"pause",              NetDiscovery::ActionId::Pause},
        {"pause_media",        NetDiscovery::ActionId::Pause},

        {"stop",               NetDiscovery::ActionId::Stop},
        {"stop_media",         NetDiscovery::ActionId::Stop},

        {"next",               NetDiscovery::ActionId::Next},
        {"skip",               NetDiscovery::ActionId::Next},
        {"next_track",         NetDiscovery::ActionId::Next},

        {"previous",           NetDiscovery::ActionId::Previous},
        {"back",               NetDiscovery::ActionId::Previous},
        {"prev",               NetDiscovery::ActionId::Previous},
        {"prev_track",         NetDiscovery::ActionId::Previous},

        {"seek",               NetDiscovery::ActionId::Seek},

        // Input & Key Controls
        {"select_input",       NetDiscovery::ActionId::SelectInput},
        {"change_input",       NetDiscovery::ActionId::SelectInput},
        {"input",              NetDiscovery::ActionId::SelectInput},

        {"send_key",           NetDiscovery::ActionId::SendKey},
        {"press_button",       NetDiscovery::ActionId::SendKey},
        {"key_press",          NetDiscovery::ActionId::SendKey}
    };

    auto it = s_aliasMap.find(lowerIntent);
    if (it != s_aliasMap.end()) {
        return it->second;
    }

    return NetDiscovery::ActionId::Unknown;
}

} // namespace semantic

