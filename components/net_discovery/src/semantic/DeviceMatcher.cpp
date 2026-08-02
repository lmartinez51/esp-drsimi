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

// Simple Levenshtein distance for fuzzy matching using zero-heap stack arrays
static int LevenshteinDistance(const std::string& s1, const std::string& s2) {
    size_t m = s1.size();
    size_t n = s2.size();
    if (m == 0) return static_cast<int>(n);
    if (n == 0) return static_cast<int>(m);
    if (m > 64) m = 64;
    if (n > 64) n = 64;

    int v0[65];
    int v1[65];
    for (size_t j = 0; j <= n; j++) v0[j] = static_cast<int>(j);

    for (size_t i = 0; i < m; i++) {
        v1[0] = static_cast<int>(i + 1);
        for (size_t j = 0; j < n; j++) {
            int deletionCost = v0[j + 1] + 1;
            int insertionCost = v1[j] + 1;
            int substitutionCost = (s1[i] == s2[j]) ? v0[j] : (v0[j] + 1);
            v1[j + 1] = std::min({deletionCost, insertionCost, substitutionCost});
        }
        for (size_t j = 0; j <= n; j++) v0[j] = v1[j];
    }
    return v0[n];
}

std::string DeviceMatcher::StripSpecialChars(const std::string& input) {
    std::string clean;
    clean.reserve(input.size());

    bool lastWasSpace = true;
    for (unsigned char c : input) {
        if (std::isalnum(c)) {
            clean.push_back(static_cast<char>(std::tolower(c)));
            lastWasSpace = false;
        } else {
            if (!lastWasSpace) {
                clean.push_back(' ');
                lastWasSpace = true;
            }
        }
    }
    if (!clean.empty() && clean.back() == ' ') {
        clean.pop_back();
    }
    return clean;
}

std::vector<NetDiscovery::LogicalDevice> DeviceMatcher::Match(const std::string& targetDescription, const std::vector<NetDiscovery::LogicalDevice>& availableDevices) const {
    std::vector<NetDiscovery::LogicalDevice> candidates;
    std::string lowerTarget = ToLower(targetDescription);
    std::string cleanTarget = StripSpecialChars(targetDescription);

    ESP_LOGI(TAG, "==================== DEVICE RESOLVER SEARCH ====================");
    ESP_LOGI(TAG, "Requested Target   : '%s'", targetDescription.c_str());
    ESP_LOGI(TAG, "Normalized Target  : '%s'", lowerTarget.c_str());
    ESP_LOGI(TAG, "Clean Target       : '%s'", cleanTarget.c_str());
    ESP_LOGI(TAG, "Candidate Entities : %d", (int)availableDevices.size());
    ESP_LOGI(TAG, "---------------------------------------------------------------");

    int index = 0;
    for (const auto& device : availableDevices) {
        std::string lowerName = ToLower(device.displayName);
        std::string cleanName = StripSpecialChars(device.displayName);
        std::string lowerManufacturer = ToLower(device.manufacturer);
        std::string cleanManufacturer = StripSpecialChars(device.manufacturer);
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

        // Exact match (basic or clean)
        if (lowerTarget == lowerName || (!cleanTarget.empty() && cleanTarget == cleanName)) {
            score = 1.0;
            reason = "Exact Match";
            ESP_LOGI(TAG, "  Score Increment: +1.00 [Exact Name Match]");
            ESP_LOGI(TAG, "  TOTAL SCORE    : %.2f", score);
            ESP_LOGI(TAG, "  Decision       : ACCEPTED (%s)", reason.c_str());
            ESP_LOGI(TAG, "---------------------------------------------------------------");
            candidates.push_back(device);
            continue;
        }

        // Room synonym normalization for English/Spanish compatibility
        std::string synTarget = lowerTarget;
        auto replace_all = [&](const std::string& from, const std::string& to) {
            size_t start_pos = 0;
            while ((start_pos = synTarget.find(from, start_pos)) != std::string::npos) {
                synTarget.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
        };
        replace_all("recamara", "bed room");
        replace_all("recámara", "bed room");
        replace_all("cuarto", "bed room");
        replace_all("habitacion", "bed room");
        replace_all("habitación", "bed room");
        replace_all("sala", "living room");
        std::string synCleanTarget = StripSpecialChars(synTarget);

        // Substring & Synonym match
        bool nameContainsTarget = (lowerName.find(lowerTarget) != std::string::npos ||
                                   (!cleanTarget.empty() && cleanName.find(cleanTarget) != std::string::npos) ||
                                   (!synCleanTarget.empty() && cleanName.find(synCleanTarget) != std::string::npos) ||
                                   (!synCleanTarget.empty() && synCleanTarget.find(cleanName) != std::string::npos));
        bool targetContainsName = (lowerTarget.find(lowerName) != std::string::npos ||
                                   (!cleanName.empty() && cleanTarget.find(cleanName) != std::string::npos) ||
                                   (!cleanName.empty() && synCleanTarget.find(cleanName) != std::string::npos));
        bool targetContainsMfg  = (!lowerManufacturer.empty() && lowerTarget.find(lowerManufacturer) != std::string::npos) ||
                                  (!cleanManufacturer.empty() && cleanTarget.find(cleanManufacturer) != std::string::npos);

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
            if (nameContainsTarget) reason = "name contains target / synonym";
            else if (targetContainsName) reason = "target contains name";
            else if (targetContainsMfg) reason = "target contains manufacturer";

            ESP_LOGI(TAG, "  Score Increment: +0.95 [Substring / Synonym Match]");
            ESP_LOGI(TAG, "  TOTAL SCORE    : %.2f", score);
            ESP_LOGI(TAG, "  Decision       : ACCEPTED (%s)", reason.c_str());
            ESP_LOGI(TAG, "---------------------------------------------------------------");
            candidates.push_back(device);
            continue;
        }

        // Fuzzy match
        int dist = LevenshteinDistance(synTarget, lowerName);
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

    // Fallback: If no candidate matched by name, but target asks for a TV/tele, pick first SmartTV
    if (candidates.empty()) {
        bool asksForTV = (lowerTarget.find("tv") != std::string::npos ||
                          lowerTarget.find("tele") != std::string::npos ||
                          lowerTarget.find("television") != std::string::npos);
        if (asksForTV) {
            for (const auto& device : availableDevices) {
                if (device.primaryClass == NetDiscovery::PrimaryDeviceClass::SmartTV) {
                    ESP_LOGI(TAG, "  Fallback Match : ACCEPTED SmartTV '%s' for TV target '%s'",
                             device.displayName.c_str(), targetDescription.c_str());
                    candidates.push_back(device);
                    break;
                }
            }
        }
    }

    ESP_LOGI(TAG, "Total Candidates Accepted: %d", (int)candidates.size());
    ESP_LOGI(TAG, "===============================================================");

    return candidates;
}

} // namespace semantic
