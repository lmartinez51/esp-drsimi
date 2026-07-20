#include "DummyTransport.h"
#include "core/LogicalDevice.h"
#include <iostream>
#include <chrono>

namespace NetDiscovery {

TransportFamily DummyTransport::GetFamily() const {
    // This is a catch-all dummy, but we return Unknown so it doesn't mask real transports.
    // In our architecture validation phase, TransportSelector will just return this
    // for everything until real transports exist.
    return TransportFamily::Unknown;
}

ExecutionResult DummyTransport::Execute(
    const ExecutionRequest& request, 
    const ExecutionRoute& route) {
    
    auto startTime = std::chrono::steady_clock::now();

    std::cout << "\n========================================\n";
    std::cout << "Execution Request\n\n";
    
    std::cout << "Device:\n";
    std::cout << (request.device.displayName.empty() ? "Unknown Device" : request.device.displayName) << "\n\n";

    std::cout << "[DummyTransport] Simulating execution of action: " << ToString(request.action.id) << "\n";

    std::cout << "Selected Transport:\n";
    std::cout << ToString(route.transport) << "\n\n";

    std::cout << "Endpoint:\n";
    if (route.preferredEndpoint) {
        if (route.preferredEndpoint->evidence.upnp.has_value() && !route.preferredEndpoint->evidence.upnp->deviceType.empty()) {
            // Shorten deviceType for display
            std::string type = route.preferredEndpoint->evidence.upnp->deviceType;
            auto pos = type.find(":device:");
            if (pos != std::string::npos) {
                type = type.substr(pos + 8);
                auto pos2 = type.find(":");
                if (pos2 != std::string::npos) {
                    type = type.substr(0, pos2);
                }
            }
            std::cout << type << " (" << route.preferredEndpoint->ip << ")\n\n";
        } else {
            std::cout << route.preferredEndpoint->ip << "\n\n";
        }
    } else {
        std::cout << "None\n\n";
    }

    std::cout << "Metadata:\n";
    for (const auto& kv : route.metadata) {
        std::cout << kv.first << " " << kv.second << "\n";
    }
    if (route.metadata.empty()) {
        std::cout << "None\n";
    }
    std::cout << "\n";

    std::cout << "========================================\n\n";

    auto endTime = std::chrono::steady_clock::now();
    int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    ExecutionResult result;
    result.status = ExecutionStatus::UnsupportedAction;
    result.elapsedTimeMs = elapsed;
    result.transportDiagnostics.rawPayload = "Printed route info to console via DummyTransport.";
    result.status = ExecutionStatus::Success;
    
    return result;
}

} // namespace NetDiscovery
