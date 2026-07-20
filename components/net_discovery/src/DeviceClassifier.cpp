/**
 * @file DeviceClassifier.cpp
 * @brief DeviceClassifier implementation.
 */

#include "../include/DeviceClassifier.h"
#include <algorithm>

namespace NetDiscovery {

void DeviceClassifier::Classify(LogicalDevice& device) {
    device.primaryClass = PrimaryDeviceClass::Unknown;
    device.roles.clear();
    
    auto hasRole = [&device](DeviceRole role) {
        return std::find(device.roles.begin(), device.roles.end(), role) != device.roles.end();
    };
    
    auto addRole = [&device, &hasRole](DeviceRole role) {
        if (!hasRole(role)) {
            device.roles.push_back(role);
        }
    };
    
    // Evaluate normalized services to determine roles (short names from ProtocolNormalizer)
    for (const auto& svc : device.normalizedServices) {
        if (svc.name == "RenderingControl" || svc.name == "AVTransport") {
            addRole(DeviceRole::MediaRenderer);
        } else if (svc.name == "ContentDirectory") {
            addRole(DeviceRole::MediaServer);
        } else if (svc.name == "ConnectionManager") {
            // ConnectionManager alone is ambiguous: appears in both MediaRenderer and MediaServer.
            // Only add MediaRenderer here if AVTransport/RenderingControl is also present.
            // (We add it tentatively; overriding logic below may promote to SmartTV anyway.)
            addRole(DeviceRole::MediaRenderer);
        } else if (svc.name == "dial") {
            addRole(DeviceRole::DIALReceiver);
        } else if (svc.name == "RemoteControlReceiver") {
            addRole(DeviceRole::RemoteControlTarget);
        } else if (svc.name == "WANDevice" || svc.name == "WANConnectionDevice"
                   || svc.name == "InternetGatewayDevice" || svc.name == "WANCommonInterfaceConfig"
                   || svc.name == "WANIPConnection" || svc.name == "WANIPv6FirewallControl"
                   || svc.name == "WANPPPConnection" || svc.name == "Layer3Forwarding") {
            addRole(DeviceRole::NetworkRouter);
        }
        // Also match 'MediaRenderer' / 'MediaServer' short names that come through
        // when a device-type URN is normalised (e.g. from SSDP USN or XML deviceType field).
        else if (svc.name == "MediaRenderer") {
            addRole(DeviceRole::MediaRenderer);
        } else if (svc.name == "MediaServer") {
            addRole(DeviceRole::MediaServer);
        }
    }

    // Also scan raw device-type signatures for role evidence.
    // This catches cases where service list is empty but device types were stored
    // from SSDP USN announcements or XML <deviceType> elements.
    for (const auto& dt : device.signature.deviceTypes) {
        if (dt.find("InternetGatewayDevice") != std::string::npos
            || dt.find("WANDevice") != std::string::npos
            || dt.find("WANConnectionDevice") != std::string::npos) {
            addRole(DeviceRole::NetworkRouter);
        } else if (dt.find("MediaRenderer") != std::string::npos) {
            addRole(DeviceRole::MediaRenderer);
        } else if (dt.find("MediaServer") != std::string::npos) {
            addRole(DeviceRole::MediaServer);
        } else if (dt.find("RemoteControlReceiver") != std::string::npos) {
            addRole(DeviceRole::RemoteControlTarget);
        } else if (dt.find("dialreceiver") != std::string::npos
                   || dt.find("dial-multiscreen") != std::string::npos) {
            addRole(DeviceRole::DIALReceiver);
        }
    }
    
    // Check specific namespaces and roles first (SmartTV overrides MediaRenderer)
    bool hasSamsungService = false;
    for (const auto& ns : device.signature.namespaces) {
        if (ns.find("samsung.com") != std::string::npos || ns.find("samsung") != std::string::npos) {
            hasSamsungService = true;
            break;
        }
    }
    
    std::string mfg = device.manufacturer;
    std::transform(mfg.begin(), mfg.end(), mfg.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    
    if (hasRole(DeviceRole::NetworkRouter)) {
        device.primaryClass = PrimaryDeviceClass::InternetGateway;
    } else if (hasSamsungService || (mfg.find("samsung") != std::string::npos && (hasRole(DeviceRole::RemoteControlTarget) || hasRole(DeviceRole::MediaRenderer) || hasRole(DeviceRole::DIALReceiver)))) {
        device.primaryClass = PrimaryDeviceClass::SmartTV;
    } else if (hasRole(DeviceRole::DIALReceiver) && mfg.find("roku") != std::string::npos) {
        device.primaryClass = PrimaryDeviceClass::StreamingDevice;
    }
    
    // Fallback to generic UPnP device types if not overridden
    if (device.primaryClass == PrimaryDeviceClass::Unknown) {
        for (const auto& dt : device.signature.deviceTypes) {
            if (dt.find("InternetGatewayDevice") != std::string::npos || dt.find("WANDevice") != std::string::npos) {
                device.primaryClass = PrimaryDeviceClass::InternetGateway;
                break;
            } else if (dt.find("MediaRenderer") != std::string::npos) {
                device.primaryClass = PrimaryDeviceClass::MediaRenderer;
                break;
            } else if (dt.find("MediaServer") != std::string::npos) {
                device.primaryClass = PrimaryDeviceClass::MediaServer;
                break;
            }
        }
    }
    
    // Fallback to roles if still unknown
    if (device.primaryClass == PrimaryDeviceClass::Unknown) {
        if (hasRole(DeviceRole::MediaRenderer)) {
            device.primaryClass = PrimaryDeviceClass::MediaRenderer;
        } else if (hasRole(DeviceRole::MediaServer)) {
            device.primaryClass = PrimaryDeviceClass::MediaServer;
        }
    }
}

} // namespace NetDiscovery
