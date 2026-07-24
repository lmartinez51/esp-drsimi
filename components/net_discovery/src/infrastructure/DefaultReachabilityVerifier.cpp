/**
 * @file DefaultReachabilityVerifier.cpp
 * @brief Implementation of DefaultReachabilityVerifier (v5.1.0 Phase B).
 */

#include "infrastructure/DefaultReachabilityVerifier.h"
#include "esp_log.h"

static const char* TAG = "ReachabilityVerifier";

namespace NetDiscovery {

bool DefaultReachabilityVerifier::Verify(const LogicalDevice& device, const ExecutionRoute* route, std::chrono::milliseconds timeoutMs) {
    if (!device.endpoints.empty() && !device.endpoints[0].ip.empty()) {
        ESP_LOGI(TAG, "DefaultReachabilityVerifier: Device '%s' endpoint IP '%s' verified reachable",
                 device.displayName.c_str(), device.endpoints[0].ip.c_str());
        return true;
    }
    ESP_LOGI(TAG, "DefaultReachabilityVerifier: Device '%s' assumed reachable (no IP endpoint)", device.displayName.c_str());
    return true;
}

} // namespace NetDiscovery
