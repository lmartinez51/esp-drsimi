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

    PowerStateReachabilityTrust ReachabilityTrust() const override {
        return PowerStateReachabilityTrust::Confirmed;
    }

    std::vector<std::string> SupportedManufacturers() const override {
        return {"Samsung", "Samsung Electronics"};
    }

    std::vector<Capability> SupportedCapabilities() const override {
        return {
            Capability::PowerControl,
            Capability::VolumeControl,
            Capability::Mute,
            Capability::InputSelection
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
        // 1. Buscar en servicios normalizados (incluyendo DIAL)
        for (const auto& svc : device.normalizedServices) {
            if (svc.name == "RemoteControlReceiver" || 
                svc.name == "MultiScreenService" || 
                svc.name == "DIAL" || 
                svc.name == "dial") {
                return true;
            }
        }

        // 2. Buscar en tipos de dispositivo en la firma
        for (const auto& dt : device.signature.deviceTypes) {
            if (dt.find("RemoteControlReceiver") != std::string::npos ||
                dt.find("dial") != std::string::npos) {
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
        
        // Handle Application Launching via DIAL
        if (action.id == ActionId::LaunchApplication) {
            ExecutionRoute route;
            // IMPORTANTE: Iterar por índice/referencia directa al vector original (device.endpoints)
            // para evitar punteros colgados (dangling pointers) en el stack.
            for (size_t i = 0; i < device.endpoints.size(); ++i) {
                const auto& ep = device.endpoints[i];
                std::string targetAppUrl = "";
                bool isDiscovered = false;

                // 1. Intentar usar la URL descubierta dinámicamente en SSDP/UPnP
                if (ep.evidence.upnp.has_value()) {
                    if (!ep.evidence.upnp->applicationUrl.empty()) {
                        targetAppUrl = ep.evidence.upnp->applicationUrl;
                        isDiscovered = true;
                    }
                }

                // 2. Fallback empírico reservado exclusivamente para Samsung TV
                // NOTA: Derivado de observaciones en la red local para esta TV específica.
                if (targetAppUrl.empty() && !ep.ip.empty()) {
                    targetAppUrl = "http://" + ep.ip + ":8080/ws/app/";
                    isDiscovered = false;
                }

                if (!targetAppUrl.empty()) {
                    route.transport = TransportFamily::DIAL;
                    // FIX CLAVE: Apuntar al elemento real del vector de la entidad (RAM estable)
                    route.preferredEndpoint = &device.endpoints[i];
                    route.metadata["Application-URL"] = targetAppUrl;

                    if (g_verbose) {
                        if (isDiscovered) {
                            std::cout << "[DIAL] Using discovered Application-URL: " << targetAppUrl << "\n";
                        } else {
                            std::cout << "[DIAL] Application-URL missing, using empirical IP-based fallback guess: " << targetAppUrl << "\n";
                        }
                    }
                    return route;
                }
            }
            return std::nullopt;
        }

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
                return std::nullopt;
            }
        } else if (action.id == ActionId::PowerOff) {
            keyName = "KEY_POWER";
        } else if (action.id == ActionId::SendKey) {
            keyName = "KEY_UNKNOWN";
        } else if (action.id == ActionId::VolumeUp) {
            keyName = "KEY_VOLUP";
        } else if (action.id == ActionId::VolumeDown) {
            keyName = "KEY_VOLDOWN";
        } else if (action.id == ActionId::SetVolume) {
            keyName = "KEY_VOLDOWN";
        } else if (action.id == ActionId::Mute) {
            keyName = "KEY_MUTE";
        }

        if (!keyName.empty()) {
            ExecutionRoute route;
            route.transport = TransportFamily::WebSocket;
            
            std::string hostIp = device.primaryIp;
            if (hostIp.empty()) {
                for (const auto& ep : device.endpoints) {
                    if (!ep.ip.empty()) {
                        hostIp = ep.ip;
                        break;
                    }
                }
            }
            route.metadata["WebSocket-Host"] = hostIp;
            route.metadata["WebSocket-Port"] = "8001";
            route.strategy = std::make_shared<Strategy::SamsungWebSocketStrategy>();
            
            // FIX CLAVE 2: Eliminar el dangling pointer en el bloque WebSocket
            for (size_t i = 0; i < device.endpoints.size(); ++i) {
                const auto& ep = device.endpoints[i];
                if (ep.evidence.upnp.has_value() && ep.evidence.upnp->deviceType.find("RemoteControlReceiver") != std::string::npos) {
                    route.preferredEndpoint = &device.endpoints[i];
                    return route;
                }
            }
            
            if (!device.endpoints.empty()) {
                route.preferredEndpoint = &device.endpoints[0];
                return route;
            }
            return std::nullopt;
        }

        return std::nullopt;
    }

};

} // namespace NetDiscovery
