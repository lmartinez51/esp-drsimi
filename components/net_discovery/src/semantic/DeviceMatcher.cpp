#include "semantic/DeviceMatcher.h"
#include <algorithm>
#include <cctype>

namespace semantic {

static std::string ToLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
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

    for (const auto& device : availableDevices) {
        std::string lowerName = ToLower(device.displayName);
        std::string lowerManufacturer = ToLower(device.manufacturer);
        std::string lowerModel = ToLower(device.model);

        // Exact match
        if (lowerTarget == lowerName) {
            candidates.push_back(device);
            continue;
        }

        // Substring match
        if (lowerName.find(lowerTarget) != std::string::npos ||
            lowerTarget.find(lowerName) != std::string::npos ||
            lowerTarget.find(lowerManufacturer) != std::string::npos) {
            candidates.push_back(device);
            continue;
        }

        // Fuzzy match
        int dist = LevenshteinDistance(lowerTarget, lowerName);
        if (dist <= 3) {
            candidates.push_back(device);
        }
    }

    return candidates;
}

} // namespace semantic
