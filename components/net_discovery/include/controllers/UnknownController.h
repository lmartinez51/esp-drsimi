/**
 * @file UnknownController.h
 * @brief Fallback controller when no others match.
 */

#pragma once

#include "../IDeviceController.h"

namespace NetDiscovery {

class UnknownController : public IDeviceController {
public:
    std::string ControllerName() const override {
        return "UnknownController";
    }

    std::vector<std::string> SupportedManufacturers() const override {
        return {}; 
    }

    std::vector<Capability> SupportedCapabilities() const override {
        return {};
    }

    bool IsMatch(const LogicalDevice& /*device*/) const override {
        return true;
    }

    ResolutionDiagnostics Evaluate(const LogicalDevice& /*device*/) const override {
        ResolutionDiagnostics diag;
        diag.score = 1; // Always matches, but with lowest possible score
        diag.scoreBreakdown.push_back({"Fallback matching", 1});
        diag.reason = "Fallback controller.";
        return diag;
    }

    bool ValidateEndpoints(const LogicalDevice& /*device*/) const override {
        return true;
    }

    std::optional<ExecutionRoute> GetExecutionRoute(
        const LogicalDevice& device, 
        const ActionDescriptor& action) const override {
        
        // TEMPORARY WIRING for Phase 8 -> 9 Demo 
        // Force DIAL transport route for Application Launching
        if (action.id == ActionId::LaunchApplication) {
            ExecutionRoute route;
            route.transport = TransportFamily::DIAL;
            
            // Find a DIAL endpoint
            for (const auto& ep : device.endpoints) {
                if (ep.evidence.upnp.has_value()) {
                    bool isDial = false;
                    for (const auto& svc : ep.evidence.upnp->services) {
                        if (svc.serviceType.find("dial-multiscreen") != std::string::npos) {
                            isDial = true;
                            break;
                        }
                    }
                    if (isDial || ep.evidence.upnp->applicationUrl.length() > 0) {
                        route.preferredEndpoint = &ep;
                        if (!ep.evidence.upnp->applicationUrl.empty()) {
                            route.metadata["Application-URL"] = ep.evidence.upnp->applicationUrl;
                        }
                        return route;
                    }
                }
            }
        } else if (action.id == ActionId::CheckReachable) {
            ExecutionRoute route;
            route.transport = TransportFamily::Unknown;
            if (!device.endpoints.empty()) {
                route.preferredEndpoint = &device.endpoints[0];
            }
            return route;
        }
        
        return std::nullopt;
    }
};

} // namespace NetDiscovery
