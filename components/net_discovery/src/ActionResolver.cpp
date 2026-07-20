/**
 * @file ActionResolver.cpp
 * @brief ActionResolver implementation.
 */

#include "../include/ActionResolver.h"
#include <algorithm>

namespace NetDiscovery {

void ActionResolver::Resolve(LogicalDevice& device) {
    device.actions.clear();
    device.capabilityProfiles.clear();
    
    auto addAction = [&device](ActionId actionId) {
        // Prevent duplicates in actions list
        for (const auto& a : device.actions) {
            if (a.id == actionId) return;
        }
        ActionDescriptor desc;
        desc.id = actionId;
        device.actions.push_back(desc);
    };

    auto addCapabilityProfile = [&device, &addAction](Capability cap, std::vector<ActionId> supportedActions) {
        CapabilityProfile profile;
        profile.capability = cap;
        profile.globalConstraints = static_cast<uint32_t>(ExecutionConstraint::None);
        
        for (auto actionId : supportedActions) {
            SupportedActionProfile sap;
            sap.actionId = actionId;
            sap.supportState = SupportState::Supported; // By default assume supported if we resolve it here
            sap.constraints = static_cast<uint32_t>(ExecutionConstraint::None);
            sap.reason = ConstraintReason::None;
            profile.supportedActions.push_back(sap);

            // Also add to the raw actions list for execution routing
            addAction(actionId);
        }

        device.capabilityProfiles.push_back(profile);
    };
    
    for (const auto& cap : device.capabilities) {
        switch (cap) {
            case Capability::PowerControl:
                addCapabilityProfile(cap, {ActionId::PowerOn, ActionId::PowerOff});
                break;
            case Capability::VolumeControl:
                addCapabilityProfile(cap, {ActionId::VolumeUp, ActionId::VolumeDown, ActionId::SetVolume, ActionId::GetVolume});
                break;
            case Capability::Mute:
                addCapabilityProfile(cap, {ActionId::Mute, ActionId::Unmute});
                break;
            case Capability::MediaPlayback:
                addCapabilityProfile(cap, {ActionId::Play, ActionId::Pause, ActionId::Stop});
                break;
            case Capability::MediaTransport:
                addCapabilityProfile(cap, {ActionId::Next, ActionId::Previous, ActionId::Seek});
                break;
            case Capability::ApplicationLaunching:
                addCapabilityProfile(cap, {ActionId::LaunchApplication});
                break;
            case Capability::InputSelection:
                addCapabilityProfile(cap, {ActionId::SelectInput});
                break;
            case Capability::RemoteControl:
                addCapabilityProfile(cap, {ActionId::SendKey});
                break;
            default:
                break;
        }
    }
}

} // namespace NetDiscovery
