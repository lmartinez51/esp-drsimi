/**
 * @file DIALTransport.cpp
 * @brief Communication transport for the DIAL protocol.
 */

#include "../../include/transports/DIALTransport.h"
#include "esp_log.h"
#include <chrono>
#include <memory>

static const char* TAG = "DIALTransport";

namespace NetDiscovery {

ExecutionResult DIALTransport::Execute(const ExecutionRequest& request, 
                                       const ExecutionRoute& route) {
    auto startTime = std::chrono::steady_clock::now();
    ExecutionResult result;

    // FIX 2: Reemplazo absoluto de std::cout por ESP_LOGI para evitar deadlocks de Newlib
    ESP_LOGI(TAG, "Execute() called for %s", ToString(route.transport).c_str());
    ESP_LOGI(TAG, "Route metadata Application-URL: %s", (route.metadata.count("Application-URL") ? route.metadata.at("Application-URL").c_str() : "MISSING"));

    if (route.transport != TransportFamily::DIAL) {
        ESP_LOGW(TAG, "Bailing early: Non-DIAL route.");
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "DIALTransport received non-DIAL route.";
        return result;
    }

    if (!route.preferredEndpoint) {
        ESP_LOGW(TAG, "Bailing early: No preferred endpoint in route.");
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "No DIAL endpoint provided in route.";
        return result;
    }
    ESP_LOGI(TAG, "Endpoint IP: %s", route.preferredEndpoint->ip.c_str());

    std::string appName;
    auto appNameIt = request.parameters.find("name");
    if (appNameIt != request.parameters.end()) {
        appName = appNameIt->second;
    } else {
        ESP_LOGW(TAG, "Bailing early: No AppName ('name') in request parameters.");
        result.status = ExecutionStatus::ExecutionFailed;
        result.errorMessage = "No AppName provided in request parameters (key: 'name').";
        return result;
    }

    std::string applicationUrl;
    auto appUrlIt = route.metadata.find("Application-URL");
    if (appUrlIt != route.metadata.end()) {
        applicationUrl = appUrlIt->second;
    }

    // FIX 1: Blindaje Estricto del Stack (6144). El cliente HTTP masivo se va a Memoria Dinámica
    auto client = std::make_unique<HttpClient>();

    if (applicationUrl.empty()) {
        // Case B: We must discover the Application-URL dynamically
        if (!route.preferredEndpoint->evidence.upnp.has_value() || 
            route.preferredEndpoint->evidence.upnp->locationUrl.empty()) {
            ESP_LOGW(TAG, "Bailing early: No Location URL available to fetch Application-URL.");
            result.status = ExecutionStatus::TransportUnavailable;
            result.errorMessage = "No Location URL available to fetch Application-URL.";
            return result;
        }

        std::string locationUrl = route.preferredEndpoint->evidence.upnp->locationUrl;
        ESP_LOGI(TAG, "Making HTTP GET to: %s", locationUrl.c_str());
        
        // Uso de puntero al Heap
        auto ddResOpt = client->Get(locationUrl);
        if (!ddResOpt.has_value()) {
            ESP_LOGE(TAG, "Bailing early: HTTP GET failed.");
            result.status = ExecutionStatus::TransportUnavailable;
            result.errorMessage = "HTTP GET failed for DIAL discovery.";
            return result;
        }
        HttpResponse ddRes = ddResOpt.value();
        auto it = ddRes.headers.find("Application-URL");
        if (it != ddRes.headers.end()) {
            applicationUrl = it->second;
            ESP_LOGI(TAG, "Extracted Application-URL from headers: %s", applicationUrl.c_str());
        } else {
            ESP_LOGE(TAG, "Bailing early: Device did not return Application-URL header.");
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
    
    // Uso de puntero al Heap para ejecutar la acción
    auto postResOpt = client->Post(launchUrl, "", dialHeaders);
    if (!postResOpt.has_value()) {
        ESP_LOGE(TAG, "HTTP POST failed.");
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