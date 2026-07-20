/**
 * @file SamsungController.h
 * @brief Controller for older Samsung Smart TVs.
 */

#pragma once

#include "../IDeviceController.h"
#include "../transports/websocket/SamsungWebSocketStrategy.h"
#include <algorithm>

namespace NetDiscovery {

class SamsungController : public IDeviceController {
public:
    std::string ControllerName() const override {
        return "SamsungController";
    }

    std::vector<std::string> SupportedManufacturers() const override {
        return {"Samsung", "Samsung Electronics"};
    }

    std::vector<Capability> SupportedCapabilities() const override {
        return {
            Capability::PowerControl,
            Capability::VolumeControl,
            Capability::Mute,
            Capability::InputSelection,
            Capability::ApplicationLaunching
        };
    }

    bool IsMatch(const LogicalDevice& device) const override {
        if (device.primaryClass != PrimaryDeviceClass::SmartTV) {
            return false;
        }

        std::string mfg = device.manufacturer;
        std::transform(mfg.begin(), mfg.end(), mfg.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (mfg.find("samsung") != std::string::npos) {
            return true;
        }
        
        for (const auto& svc : device.normalizedServices) {
            if (svc.domain == "samsung.com" || svc.domain == "samsung") {
                return true;
            }
        }
        
        return false;
    }

    ResolutionDiagnostics Evaluate(const LogicalDevice& device) const override {
        ResolutionDiagnostics diag;
        diag.score = 0;

        // Manufacturer confirmation is the strongest signal for a vendor-specific
        // controller. Score is set high enough to always outrank generic protocol
        // matches (GenericDLNAController maxes at 100 on a fully-equipped device).
        std::string mfg = device.manufacturer;
        std::transform(mfg.begin(), mfg.end(), mfg.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (mfg.find("samsung") != std::string::npos) {
            diag.score += 100;
            diag.scoreBreakdown.push_back({"Samsung Manufacturer (confirmed)", 100});
            diag.matchedManufacturer = true;
        }

        for (const auto& svc : device.normalizedServices) {
            if (svc.domain == "samsung.com" || svc.domain == "samsung") {
                diag.score += 30;
                diag.scoreBreakdown.push_back({"Samsung Namespace", 30});
                break;
            }
        }

        for (const auto& svc : device.normalizedServices) {
            if (svc.name == "RenderingControl") {
                diag.score += 5;
                diag.scoreBreakdown.push_back({"RenderingControl", 5});
            } else if (svc.name == "AVTransport") {
                diag.score += 5;
                diag.scoreBreakdown.push_back({"AVTransport", 5});
            } else if (svc.name == "dial") {
                diag.score += 2;
                diag.scoreBreakdown.push_back({"DIAL Service", 2});
            }
        }

        if (diag.score > 0) {
            diag.reason = "Samsung manufacturer or specific services matched.";
        } else {
            diag.reason = "Not a Samsung device.";
        }

        return diag;
    }

    bool ValidateEndpoints(const LogicalDevice& device) const override {
        for (const auto& svc : device.normalizedServices) {
            if (svc.name == "RemoteControlReceiver" || svc.name == "MultiScreenService") {
                return true;
            }
        }
        for (const auto& dt : device.signature.deviceTypes) {
            if (dt.find("RemoteControlReceiver") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    std::vector<ActionDescriptor> VendorActions() const override {
        return {
            {ActionId::PowerOn, "Power On", ActionCategory::Power, {}, false, ""},
            {ActionId::PowerOff, "Power Off", ActionCategory::Power, {}, false, ""}
        };
    }

    std::optional<ExecutionRoute> GetExecutionRoute(
        const LogicalDevice& device, 
        const ActionDescriptor& action) const override {
        
        // 1. Explicit Allowlist of actions this controller natively implements
        std::string keyName = "";
        if (action.id == ActionId::PowerOn) {
            if (device.signature.mac.has_value() && !device.signature.mac->empty()) {
                ExecutionRoute route;
                route.transport = TransportFamily::WakeOnLAN;
                route.metadata["Target-MAC"] = device.signature.mac.value();
                if (!device.endpoints.empty()) {
                    route.preferredEndpoint = &device.endpoints[0];
                }
                return route;
            } else {
                // Cannot WakeOnLAN without MAC address
                return std::nullopt;
            }
        } else if (action.id == ActionId::PowerOff) {
            keyName = "KEY_POWER";
        } else if (action.id == ActionId::SendKey) {
            keyName = "KEY_UNKNOWN"; // We would normally extract the key from params
        } else if (action.id == ActionId::CheckReachable) {
            ExecutionRoute route;
            route.transport = TransportFamily::Unknown;
            if (!device.endpoints.empty()) {
                route.preferredEndpoint = &device.endpoints[0];
            }
            return route;
        } else if (action.id == ActionId::VolumeUp) {
            keyName = "KEY_VOLUP";
        } else if (action.id == ActionId::VolumeDown) {
            keyName = "KEY_VOLDOWN";
        } else if (action.id == ActionId::SetVolume) {
            keyName = "KEY_VOLDOWN"; // Simplification since SetVolume needs semantic mapping to keys
        } else if (action.id == ActionId::Mute) {
            keyName = "KEY_MUTE";
        }

        if (!keyName.empty()) {
            ExecutionRoute route;
            route.transport = TransportFamily::WebSocket;
            route.metadata["WebSocket-Host"] = device.primaryIp;
            route.metadata["WebSocket-Port"] = "8001";
            
            // Assign the strategy to handle request building and response processing
            route.strategy = std::make_shared<Strategy::SamsungWebSocketStrategy>();
            
            for (const auto& ep : device.endpoints) {
                if (ep.evidence.upnp.has_value() && ep.evidence.upnp->deviceType.find("RemoteControlReceiver") != std::string::npos) {
                    route.preferredEndpoint = &ep;
                    return route;
                }
            }
            if (!device.endpoints.empty()) {
                route.preferredEndpoint = &device.endpoints[0];
                return route;
            }
            return std::nullopt; // No valid endpoint found even for supported action
        }

        // 2. Deny-by-Default for EVERYTHING else
        return std::nullopt;
    }

};

} // namespace NetDiscovery
