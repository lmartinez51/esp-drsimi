#pragma once

#include <string>
#include <vector>
#include "core/LogicalDevice.h"

namespace semantic {

class DeviceMatcher {
public:
    /**
     * @brief Matches a target description to a set of LogicalDevices based on fuzzy matching.
     * @param targetDescription e.g. "The TV"
     * @param availableDevices The list of devices in the KnowledgeStore.
     * @return A list of candidate devices that match the text description.
     */
    std::vector<NetDiscovery::LogicalDevice> Match(const std::string& targetDescription, const std::vector<NetDiscovery::LogicalDevice>& availableDevices) const;

    /**
     * @brief Normalizes input by lowercasing, stripping non-alphanumeric characters, and collapsing whitespace.
     */
    static std::string StripSpecialChars(const std::string& input);
};

} // namespace semantic
