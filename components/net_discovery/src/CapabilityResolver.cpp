/**
 * @file CapabilityResolver.cpp
 * @brief CapabilityResolver implementation.
 */

#include "../include/CapabilityResolver.h"
#include <algorithm>

namespace NetDiscovery {

void CapabilityResolver::Resolve(LogicalDevice& device) {
    device.capabilities.clear();
    
    auto addCapability = [&device](Capability cap) {
        if (std::find(device.capabilities.begin(), device.capabilities.end(), cap) == device.capabilities.end()) {
            device.capabilities.push_back(cap);
        }
    };
    
    for (const auto& svc : device.normalizedServices) {
        if (svc.name == "RenderingControl") {
            addCapability(Capability::VolumeControl);
            addCapability(Capability::Mute);
        } else if (svc.name == "AVTransport") {
            addCapability(Capability::MediaPlayback);
            addCapability(Capability::MediaTransport);
        } else if (svc.name == "ConnectionManager") {
            addCapability(Capability::InputSelection);
        } else if (svc.name == "dial") {
            addCapability(Capability::ApplicationLaunching);
        } else if (svc.name == "RemoteControlReceiver") {
            addCapability(Capability::RemoteControl);
        }
    }

    // Role-based capability resolution (catches cases where service list
    // is sparse but roles were assigned from device-type evidence).
    for (const auto& role : device.roles) {
        if (role == DeviceRole::DIALReceiver) {
            addCapability(Capability::ApplicationLaunching);
        } else if (role == DeviceRole::RemoteControlTarget) {
            addCapability(Capability::PowerControl);
            addCapability(Capability::VolumeControl);
            addCapability(Capability::Mute);
            addCapability(Capability::InputSelection);
        } else if (role == DeviceRole::MediaRenderer) {
            addCapability(Capability::MediaPlayback);
            addCapability(Capability::MediaTransport);
            addCapability(Capability::VolumeControl);
            addCapability(Capability::Mute);
        }
    }
}

} // namespace NetDiscovery
