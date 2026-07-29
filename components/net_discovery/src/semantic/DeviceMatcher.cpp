#include "semantic/DeviceMatcher.h"
#include "esp_log.h"
#include <algorithm>
#include <cctype>

static const char* TAG = "DeviceMatcher";

namespace semantic {

static std::string ToLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ 
                       return (c == '_') ? ' ' : static_cast<char>(std::tolower(c)); 
                   });
    return result;
}

// Simple Levenshtein distance for fuzzy matching
static int LevenshteinDistance(const std::string& s1, const std::string& s2) {
    size_t m = s1.size();
    size_t n = s2.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (size_t i = 0; i <= m; i++) dp[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= n; j++) dp[0][j] = static_cast<int>(j);

    for (size_t i = 1; i <= m; i++) {
        for (size_t j = 1; j <= n; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[m][n];
}

std::vector<NetDiscovery::LogicalDevice> DeviceMatcher::Match(const std::string& targetDescription, const std::vector<NetDiscovery::LogicalDevice>& availableDevices) const {
    std::vector<NetDiscovery::LogicalDevice> candidates;
    std::string lowerTarget = ToLower(targetDescription);

    ESP_LOGI(TAG, "==================== DEVICE RESOLVER SEARCH ====================");
    ESP_LOGI(TAG, "Requested Target   : '%s'", targetDescription.c_str());
    ESP_LOGI(TAG, "Normalized Target  : '%s'", lowerTarget.c_str());
    ESP_LOGI(TAG, "Candidate Entities : %d", (int)availableDevices.size());
    ESP_LOGI(TAG, "---------------------------------------------------------------");

    int index = 0;
    for (const auto& device : availableDevices) {
        std::string lowerName = ToLower(device.displayName);
        std::string lowerManufacturer = ToLower(device.manufacturer);
        std::string lowerModel = ToLower(device.model);

        ESP_LOGI(TAG, "Evaluating Entity #%d:", index++);
        ESP_LOGI(TAG, "  FriendlyName : '%s'", device.displayName.c_str());
        ESP_LOGI(TAG, "  Manufacturer : '%s'", device.manufacturer.c_str());
        ESP_LOGI(TAG, "  Model        : '%s'", device.model.c_str());
        ESP_LOGI(TAG, "  PrimaryClass : %s", NetDiscovery::ToString(device.primaryClass).c_str());

        ESP_LOGI(TAG, "  Capabilities:");
        if (device.capabilities.empty()) {
            ESP_LOGI(TAG, "    (None)");
        } else {
            for (const auto& cap : device.capabilities) {
                ESP_LOGI(TAG, "    %s", NetDiscovery::ToString(cap).c_str());
            }
        }

        double score = 0.0;
        std::string reason = "No match";

        // Exact match
        if (lowerTarget == lowerName) {
            score = 1.0;
            reason = "Exact Match (lowerTarget == lowerName)";
            ESP_LOGI(TAG, "  Score Increment: +1.00 [Exact Name Match]");
            ESP_LOGI(TAG, "  TOTAL SCORE    : %.2f", score);
            ESP_LOGI(TAG, "  Decision       : ACCEPTED (%s)", reason.c_str());
            ESP_LOGI(TAG, "---------------------------------------------------------------");
            candidates.push_back(device);
            continue;
        }

        // Substring match
        bool nameContainsTarget = (lowerName.find(lowerTarget) != std::string::npos);
        bool targetContainsName = (lowerTarget.find(lowerName) != std::string::npos);
        bool targetContainsMfg = (!lowerManufacturer.empty() && lowerTarget.find(lowerManufacturer) != std::string::npos);

        if (nameContainsTarget || targetContainsName || targetContainsMfg) {
            // Guard: Reject devices that have no capabilities and no non-rejected controllers
            bool hasValidController = false;
            for (const auto& ctrl : device.controllerCandidates) {
                if (!ctrl.isRejected && ctrl.name != "UnknownController") {
                    hasValidController = true;
                    break;
                }
            }

            if (device.capabilities.empty() && !hasValidController) {
                score = 0.0;
                reason = "Rejected: Device has no control capabilities or valid controller candidates";
                ESP_LOGI(TAG, "  TOTAL SCORE    : 0.00");
                ESP_LOGI(TAG, "  Decision       : REJECTED (%s)", reason.c_str());
                ESP_LOGI(TAG, "---------------------------------------------------------------");
                continue;
            }

            score = 0.95;
            if (nameContainsTarget) reason = "lowerName contains lowerTarget";
            else if (targetContainsName) reason = "lowerTarget contains lowerName";
            else if (targetContainsMfg) reason = "lowerTarget contains lowerManufacturer";

            ESP_LOGI(TAG, "  Score Increment: +0.95 [Substring Match]");
            ESP_LOGI(TAG, "  TOTAL SCORE    : %.2f", score);
            ESP_LOGI(TAG, "  Decision       : ACCEPTED (%s)", reason.c_str());
            ESP_LOGI(TAG, "---------------------------------------------------------------");
            candidates.push_back(device);
            continue;
        }

        // Fuzzy match
        int dist = LevenshteinDistance(lowerTarget, lowerName);
        if (dist <= 3) {
            score = 0.70 - (dist * 0.10);
            reason = "Fuzzy match distance " + std::to_string(dist);
            ESP_LOGI(TAG, "  Score Increment: +%.2f [Fuzzy Match dist=%d]", score, dist);
            ESP_LOGI(TAG, "  TOTAL SCORE    : %.2f", score);
            ESP_LOGI(TAG, "  Decision       : ACCEPTED (%s)", reason.c_str());
            ESP_LOGI(TAG, "---------------------------------------------------------------");
            candidates.push_back(device);
        } else {
            ESP_LOGI(TAG, "  TOTAL SCORE    : 0.00");
            ESP_LOGI(TAG, "  Decision       : REJECTED (No match criteria satisfied)");
            ESP_LOGI(TAG, "---------------------------------------------------------------");
        }
    }

    ESP_LOGI(TAG, "Total Candidates Accepted: %d", (int)candidates.size());
    ESP_LOGI(TAG, "===============================================================");

    return candidates;
}

} // namespace semantic
