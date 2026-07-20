/**
 * @file GenericDLNAController.h
 * @brief Generic DLNA/UPnP controller metadata.
 */

#pragma once

#include <iostream>
#include "../IDeviceController.h"
#include "../transports/soap/SOAPExecutionContext.h"
#include "../transports/soap/builders/RenderingControlBuilder.h"
#include "../transports/soap/builders/AVTransportBuilder.h"
#include "../transports/soap/builders/ConnectionManagerBuilder.h"

extern bool g_verbose;

namespace NetDiscovery {

class GenericDLNAController : public IDeviceController {
public:
    std::string ControllerName() const override {
        return "GenericDLNAController";
    }

    std::vector<std::string> SupportedManufacturers() const override {
        return {}; // Generic supports any
    }

    std::vector<Capability> SupportedCapabilities() const override {
        return {
            Capability::MediaPlayback,
            Capability::MediaTransport,
            Capability::VolumeControl,
            Capability::Mute,
            Capability::ApplicationLaunching
        };
    }

    bool IsMatch(const LogicalDevice& device) const override {
        if (device.primaryClass == PrimaryDeviceClass::InternetGateway || 
            device.primaryClass == PrimaryDeviceClass::Unknown ||
            device.primaryClass == PrimaryDeviceClass::IoTDevice) {
            return false;
        }

        if (device.primaryClass == PrimaryDeviceClass::MediaRenderer ||
            device.primaryClass == PrimaryDeviceClass::MediaServer ||
            device.primaryClass == PrimaryDeviceClass::SmartTV) {
            return true;
        }

        return false;
    }

    ResolutionDiagnostics Evaluate(const LogicalDevice& device) const override {
        ResolutionDiagnostics diag;
        diag.score = 0;

        for (const auto& svc : device.normalizedServices) {
            if (svc.domain == "schemas-upnp-org" || svc.domain == "upnp") {
                diag.score += 20;
                diag.scoreBreakdown.push_back({"UPnP Namespace", 20});
                break; // Only score once for namespace
            }
        }
        
        for (const auto& svc : device.normalizedServices) {
            if (svc.name == "MediaRenderer") {
                diag.score += 40;
                diag.scoreBreakdown.push_back({"MediaRenderer", 40});
            } else if (svc.name == "AVTransport") {
                diag.score += 30;
                diag.scoreBreakdown.push_back({"AVTransport", 30});
            } else if (svc.name == "RenderingControl") {
                diag.score += 10;
                diag.scoreBreakdown.push_back({"RenderingControl", 10});
            } else if (svc.name == "DIAL") {
                diag.score += 30;
                diag.scoreBreakdown.push_back({"DIAL Service", 30});
            }
        }

        if (diag.score >= 50) {
            diag.reason = "Generic DLNA services detected.";
        } else {
            diag.reason = "Not a full DLNA device.";
        }

        return diag;
    }

    bool ValidateEndpoints(const LogicalDevice& device) const override {
        for (const auto& svc : device.normalizedServices) {
            if (svc.name == "RenderingControl" || svc.name == "AVTransport" || svc.name == "ContentDirectory" || svc.name == "DIAL") {
                return true;
            }
        }
        return false;
    }

    std::optional<ExecutionRoute> GetExecutionRoute(
        const LogicalDevice& device, 
        const ActionDescriptor& action) const override {
        
        ExecutionRoute route;

        // DIAL Support
        if (action.id == ActionId::LaunchApplication) {
            bool isDial = false;
            for (const auto& r : device.roles) {
                if (r == DeviceRole::DIALReceiver) {
                    isDial = true;
                    break;
                }
            }
            if (!isDial) {
                for (const auto& svc : device.normalizedServices) {
                    if (svc.name == "DIAL") {
                        isDial = true;
                        break;
                    }
                }
            }

            if (isDial) {
                route.transport = TransportFamily::DIAL;
                for (const auto& ep : device.endpoints) {
                    if (ep.evidence.upnp.has_value()) {
                        route.preferredEndpoint = &ep;
                        
                        if (!ep.evidence.upnp->applicationUrl.empty()) {
                            route.metadata["Application-URL"] = ep.evidence.upnp->applicationUrl;
                            if (g_verbose) {
                                std::cout << "[Metadata] GenericDLNAController adding Application-URL to route: " << ep.evidence.upnp->applicationUrl << "\n";
                            }
                        } else {
                            if (g_verbose) {
                                std::cout << "[Metadata] GenericDLNAController found NO Application-URL in endpoint evidence\n";
                            }
                        }
                        return route;
                    }
                }
            }
        } else if (action.id == ActionId::CheckReachable) {
            route.transport = TransportFamily::Unknown;
            if (!device.endpoints.empty()) {
                route.preferredEndpoint = &device.endpoints[0];
            }
            return route;
        }

        // Default SOAP Support
        route.transport = TransportFamily::SOAP;
        
        // Find the most appropriate service based on action category
        StandardService targetService;
        if (action.category == ActionCategory::MediaPlayback || action.category == ActionCategory::MediaTransport) {
            targetService = StandardService::AVTransport;
        } else if (action.category == ActionCategory::System || action.category == ActionCategory::Unknown) {
            targetService = StandardService::RenderingControl; // default for volume etc
        } else {
            targetService = StandardService::RenderingControl;
        }

        // 1. Find the generic service descriptor
        const ServiceDescriptor* svcDesc = device.FindFirstService(targetService);
        if (!svcDesc) {
            if (g_verbose) {
                std::cout << "[Metadata] GenericDLNAController found NO service matching standard type " << static_cast<int>(targetService) << "\n";
            }
            return std::nullopt; // Or fallback, but realistically without the service we can't execute.
        }

        // 2. Select the preferred endpoint that matches the protocol family
        for (const auto& ep : device.endpoints) {
            if (svcDesc->protocolFamily == "UPnP" && ep.evidence.upnp.has_value()) {
                route.preferredEndpoint = &ep;
                break;
            }
        }
        if (!route.preferredEndpoint && !device.endpoints.empty()) {
            route.preferredEndpoint = &device.endpoints[0];
        }

        // 3. Build the execution context using the stateless builders
        SOAPRequest soapReq;
        
        if (targetService == StandardService::RenderingControl) {
            if (action.id == ActionId::SetVolume) {
                SetVolumeRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                req.targetVolume = 15; // Hardcoded placeholder for Phase 8A
                soapReq = RenderingControlBuilder::BuildSetVolume(req);
            } else if (action.id == ActionId::GetVolume || action.id == ActionId::VolumeUp || action.id == ActionId::VolumeDown) {
                // We map VolumeUp/VolumeDown to GetVolume for now just for the Phase 8A demo 
                // until Phase 8B implements the real orchestration.
                GetVolumeRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                soapReq = RenderingControlBuilder::BuildGetVolume(req);
            } else if (action.id == ActionId::Mute) {
                SetMuteRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                req.muteStatus = true; // Hardcoded placeholder
                soapReq = RenderingControlBuilder::BuildSetMute(req);
            } else if (action.id == ActionId::Unmute) {
                SetMuteRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                req.muteStatus = false; // Hardcoded placeholder
                soapReq = RenderingControlBuilder::BuildSetMute(req);
            } else {
                return std::nullopt; // Action not supported yet
            }
        } else if (targetService == StandardService::AVTransport) {
            if (action.id == ActionId::Play) {
                PlayRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                soapReq = AVTransportBuilder::BuildPlay(req);
            } else if (action.id == ActionId::Pause) {
                PauseRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                soapReq = AVTransportBuilder::BuildPause(req);
            } else if (action.id == ActionId::Stop) {
                StopRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                soapReq = AVTransportBuilder::BuildStop(req);
            } else if (action.id == ActionId::Next) {
                NextRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                soapReq = AVTransportBuilder::BuildNext(req);
            } else if (action.id == ActionId::Previous) {
                PreviousRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                soapReq = AVTransportBuilder::BuildPrevious(req);
            } else if (action.id == ActionId::Seek) {
                SeekRequest req;
                req.instanceID = 0;
                req.controlUrl = svcDesc->controlUrl;
                req.target = "00:00:00"; // Hardcoded placeholder
                soapReq = AVTransportBuilder::BuildSeek(req);
            } else {
                return std::nullopt; // Action not supported yet
            }
        } else if (targetService == StandardService::ConnectionManager) {
            return std::nullopt;
        } else {
            return std::nullopt;
        }
        
        auto soapContext = std::make_shared<SOAPExecutionContext>(soapReq);
        route.executionContext = soapContext;

        return route;
    }
};

} // namespace NetDiscovery
