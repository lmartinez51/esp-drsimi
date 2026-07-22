/**
 * @file DIALTransport.cpp
 * @brief Communication transport for the DIAL protocol.
 */

#include "../../include/transports/DIALTransport.h"
#include "esp_log.h"
#include <chrono>
#include <iostream>

static const char* TAG = "DIALTransport";

namespace NetDiscovery {

ExecutionResult DIALTransport::Execute(const ExecutionRequest& request, 
                                       const ExecutionRoute& route) {
    auto startTime = std::chrono::steady_clock::now();
    ExecutionResult result;

    std::cout << "\n[DIALTransport] Execute() called for " << ToString(route.transport) << "\n";
    std::cout << "[DIALTransport] Route metadata Application-URL: " << (route.metadata.count("Application-URL") ? route.metadata.at("Application-URL") : "MISSING") << "\n";

    if (route.transport != TransportFamily::DIAL) {
        std::cout << "[DIALTransport] Bailing early: Non-DIAL route.\n";
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "DIALTransport received non-DIAL route.";
        return result;
    }

    if (!route.preferredEndpoint) {
        std::cout << "[DIALTransport] Bailing early: No preferred endpoint in route.\n";
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "No DIAL endpoint provided in route.";
        return result;
    }
    std::cout << "[DIALTransport] Endpoint IP: " << route.preferredEndpoint->ip << "\n";

    std::string appName;
    auto appNameIt = request.parameters.find("name");
    if (appNameIt != request.parameters.end()) {
        appName = appNameIt->second;
    } else {
        std::cout << "[DIALTransport] Bailing early: No AppName ('name') in request parameters.\n";
        result.status = ExecutionStatus::ExecutionFailed;
        result.errorMessage = "No AppName provided in request parameters (key: 'name').";
        return result;
    }

    std::string applicationUrl;
    auto appUrlIt = route.metadata.find("Application-URL");
    if (appUrlIt != route.metadata.end()) {
        applicationUrl = appUrlIt->second;
    }

    HttpClient client;

    if (applicationUrl.empty()) {
        // Case B: We must discover the Application-URL dynamically
        if (!route.preferredEndpoint->evidence.upnp.has_value() || 
            route.preferredEndpoint->evidence.upnp->locationUrl.empty()) {
            std::cout << "[DIALTransport] Bailing early: No Location URL available to fetch Application-URL.\n";
            result.status = ExecutionStatus::TransportUnavailable;
            result.errorMessage = "No Location URL available to fetch Application-URL.";
            return result;
        }

        std::string locationUrl = route.preferredEndpoint->evidence.upnp->locationUrl;
        std::cout << "[DIALTransport] Making HTTP GET to: " << locationUrl << "\n";
        auto ddResOpt = client.Get(locationUrl);
        if (!ddResOpt.has_value()) {
            std::cout << "[DIALTransport] Bailing early: HTTP GET failed.\n";
            result.status = ExecutionStatus::TransportUnavailable;
            result.errorMessage = "HTTP GET failed for DIAL discovery.";
            return result;
        }
        HttpResponse ddRes = ddResOpt.value();
        auto it = ddRes.headers.find("Application-URL");
        if (it != ddRes.headers.end()) {
            applicationUrl = it->second;
            std::cout << "[DIALTransport] Extracted Application-URL from headers: " << applicationUrl << "\n";
        } else {
            std::cout << "[DIALTransport] Bailing early: Device did not return Application-URL header.\n";
            result.status = ExecutionStatus::ProtocolError;
            result.errorMessage = "DIAL device did not return Application-URL header.";
            return result;
        }
    }

    // Ensure applicationUrl ends with '/'
    if (!applicationUrl.empty() && applicationUrl.back() != '/') {
        applicationUrl += '/';
    }

    // Case A / Executing POST
    std::string launchUrl = applicationUrl + appName;
    
    ESP_LOGI(TAG, "Application URL  : %s", applicationUrl.c_str());
    ESP_LOGI(TAG, "Application Name : %s", appName.c_str());
    ESP_LOGI(TAG, "Final URL        : %s", launchUrl.c_str());
    
    std::map<std::string, std::string> dialHeaders = {
        {"Origin", "package:com.netdiscovery"},
        {"User-Agent", "NetDiscovery/1.0"}
    };
    auto postResOpt = client.Post(launchUrl, "", dialHeaders);
    if (!postResOpt.has_value()) {
        std::cout << "[DIALTransport] HTTP POST failed.\n";
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "HTTP POST failed for DIAL launch.";
        return result;
    }
    HttpResponse postRes = postResOpt.value();
    
    ESP_LOGI(TAG, "HTTP Method      : POST");
    ESP_LOGI(TAG, "URL              : %s", launchUrl.c_str());
    ESP_LOGI(TAG, "Response Status  : %d", postRes.statusCode);
    ESP_LOGI(TAG, "Response Body    : %s", postRes.body.c_str());

    if (postRes.statusCode == 200 || postRes.statusCode == 201) {
        result.status = ExecutionStatus::Success;
        result.transportDiagnostics.rawPayload = "App launched successfully.";
    } else if (postRes.statusCode == 404) {
        result.status = ExecutionStatus::UnsupportedAction;
        result.errorMessage = "App not installed or found on device.";
    } else if (postRes.statusCode == 401 || postRes.statusCode == 403) {
        result.status = ExecutionStatus::AuthenticationRequired;
        result.errorMessage = "Authentication or pairing required to launch app.";
    } else if (postRes.statusCode == 409 || postRes.statusCode == 503) {
        result.status = ExecutionStatus::ExecutionFailed;
        result.errorMessage = "Device busy or app in invalid state (Status " + std::to_string(postRes.statusCode) + ").";
    } else {
        result.status = ExecutionStatus::ProtocolError;
        result.errorMessage = "Unexpected HTTP status: " + std::to_string(postRes.statusCode);
    }

    auto endTime = std::chrono::steady_clock::now();
    result.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    return result;
}

} // namespace NetDiscovery
