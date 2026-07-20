#include "semantic/CapabilityFilter.h"

namespace semantic {

std::vector<NetDiscovery::LogicalDevice> CapabilityFilter::Filter(const std::vector<NetDiscovery::LogicalDevice>& candidates, NetDiscovery::ActionId intent) const {
    std::vector<NetDiscovery::LogicalDevice> validDevices;

    NetDiscovery::Capability requiredCapability = NetDiscovery::Capability::Unknown;

    // Map intent to required capability
    switch (intent) {
        case NetDiscovery::ActionId::PowerOn:
        case NetDiscovery::ActionId::PowerOff:
            requiredCapability = NetDiscovery::Capability::PowerControl;
            break;
        case NetDiscovery::ActionId::VolumeUp:
        case NetDiscovery::ActionId::VolumeDown:
        case NetDiscovery::ActionId::SetVolume:
            requiredCapability = NetDiscovery::Capability::VolumeControl;
            break;
        case NetDiscovery::ActionId::Mute:
        case NetDiscovery::ActionId::Unmute:
            requiredCapability = NetDiscovery::Capability::Mute;
            break;
        case NetDiscovery::ActionId::Play:
        case NetDiscovery::ActionId::Pause:
        case NetDiscovery::ActionId::Stop:
            requiredCapability = NetDiscovery::Capability::MediaPlayback;
            break;
        case NetDiscovery::ActionId::Next:
        case NetDiscovery::ActionId::Previous:
            requiredCapability = NetDiscovery::Capability::MediaTransport;
            break;
        case NetDiscovery::ActionId::LaunchApplication:
            requiredCapability = NetDiscovery::Capability::ApplicationLaunching;
            break;
        case NetDiscovery::ActionId::SendKey:
            requiredCapability = NetDiscovery::Capability::RemoteControl;
            break;
        default:
            requiredCapability = NetDiscovery::Capability::Unknown;
            break;
    }

    if (requiredCapability == NetDiscovery::Capability::Unknown) {
        return candidates; // No specific capability required
    }

    for (const auto& device : candidates) {
        if (device.HasCapability(requiredCapability)) {
            validDevices.push_back(device);
        }
    }

    return validDevices;
}

} // namespace semantic
