#include "semantic/IntentCanonicalizerTest.h"
#include "semantic/IntentCanonicalizer.h"
#include <iostream>
#include <vector>

namespace semantic {

struct TestCase {
    std::string inputAlias;
    NetDiscovery::ActionId expectedId;
};

bool RunIntentCanonicalizerTests() {
    IntentCanonicalizer canonicalizer;
    bool allPassed = true;

    std::vector<TestCase> testCases = {
        // LaunchApplication Aliases
        {"launch_app",         NetDiscovery::ActionId::LaunchApplication},
        {"launch_application", NetDiscovery::ActionId::LaunchApplication},
        {"open_app",           NetDiscovery::ActionId::LaunchApplication},
        {"watch_netflix",      NetDiscovery::ActionId::LaunchApplication},
        {"open_netflix",       NetDiscovery::ActionId::LaunchApplication},
        {"LAUNCH_APP",         NetDiscovery::ActionId::LaunchApplication},

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

        // Mute / Audio Controls
        {"mute",               NetDiscovery::ActionId::Mute},
        {"silence",            NetDiscovery::ActionId::Mute},
        {"activate_mute",      NetDiscovery::ActionId::Mute},
        {"unmute",             NetDiscovery::ActionId::Unmute},
        {"restore_audio",      NetDiscovery::ActionId::Unmute},

        // Media Controls
        {"play",               NetDiscovery::ActionId::Play},
        {"resume",             NetDiscovery::ActionId::Play},
        {"pause",              NetDiscovery::ActionId::Pause},
        {"pause_media",        NetDiscovery::ActionId::Pause},
        {"stop",               NetDiscovery::ActionId::Stop},
        {"stop_media",         NetDiscovery::ActionId::Stop},
        {"next",               NetDiscovery::ActionId::Next},
        {"skip",               NetDiscovery::ActionId::Next},
        {"previous",           NetDiscovery::ActionId::Previous},
        {"back",               NetDiscovery::ActionId::Previous},
        {"prev",               NetDiscovery::ActionId::Previous},

        // Input & Key Controls
        {"select_input",       NetDiscovery::ActionId::SelectInput},
        {"change_input",       NetDiscovery::ActionId::SelectInput},
        {"send_key",           NetDiscovery::ActionId::SendKey},
        {"press_button",       NetDiscovery::ActionId::SendKey},

        // Negative Test Cases
        {"unknown_command_xyz",NetDiscovery::ActionId::Unknown},
        {"invalid_action",     NetDiscovery::ActionId::Unknown}
    };

    int passedCount = 0;
    int failedCount = 0;

    for (const auto& tc : testCases) {
        NetDiscovery::ActionId result = canonicalizer.Normalize(tc.inputAlias);
        if (result == tc.expectedId) {
            passedCount++;
        } else {
            failedCount++;
            allPassed = false;
            std::cout << "[REGRESSION TEST FAIL] Input: '" << tc.inputAlias 
                      << "' -> Expected ActionId: " << static_cast<int>(tc.expectedId) 
                      << " | Got: " << static_cast<int>(result) << std::endl;
        }
    }

    std::cout << "[IntentCanonicalizer Test Suite] Total: " << testCases.size() 
              << " | Passed: " << passedCount 
              << " | Failed: " << failedCount << std::endl;

    return allPassed;
}

} // namespace semantic
