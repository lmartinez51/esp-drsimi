#include "semantic/IntentCanonicalizer.h"
#include <algorithm>
#include <cctype>

namespace semantic {

static std::string ToLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return result;
}

NetDiscovery::ActionId IntentCanonicalizer::Normalize(const std::string& rawIntent) const {
    std::string lowerIntent = ToLower(rawIntent);

    if (lowerIntent == "power_on" || lowerIntent == "turn_on" || lowerIntent == "on") {
        return NetDiscovery::ActionId::PowerOn;
    } else if (lowerIntent == "power_off" || lowerIntent == "turn_off" || lowerIntent == "off") {
        return NetDiscovery::ActionId::PowerOff;
    } else if (lowerIntent == "volume_up" || lowerIntent == "louder") {
        return NetDiscovery::ActionId::VolumeUp;
    } else if (lowerIntent == "volume_down" || lowerIntent == "quieter") {
        return NetDiscovery::ActionId::VolumeDown;
    } else if (lowerIntent == "set_volume") {
        return NetDiscovery::ActionId::SetVolume;
    } else if (lowerIntent == "mute") {
        return NetDiscovery::ActionId::Mute;
    } else if (lowerIntent == "unmute") {
        return NetDiscovery::ActionId::Unmute;
    } else if (lowerIntent == "launch_application" || lowerIntent == "watch_netflix" || lowerIntent == "open_app") {
        return NetDiscovery::ActionId::LaunchApplication;
    } else if (lowerIntent == "play" || lowerIntent == "resume") {
        return NetDiscovery::ActionId::Play;
    } else if (lowerIntent == "pause") {
        return NetDiscovery::ActionId::Pause;
    } else if (lowerIntent == "stop") {
        return NetDiscovery::ActionId::Stop;
    } else if (lowerIntent == "next" || lowerIntent == "skip") {
        return NetDiscovery::ActionId::Next;
    } else if (lowerIntent == "previous" || lowerIntent == "back") {
        return NetDiscovery::ActionId::Previous;
    } else if (lowerIntent == "send_key" || lowerIntent == "press_button") {
        return NetDiscovery::ActionId::SendKey;
    } else if (lowerIntent == "check_reachable") {
        return NetDiscovery::ActionId::CheckReachable;
    }

    return NetDiscovery::ActionId::Unknown;
}

} // namespace semantic
