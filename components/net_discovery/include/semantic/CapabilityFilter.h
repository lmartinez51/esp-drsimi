#pragma once

#include <vector>
#include "../core/LogicalDevice.h"
#include "../core/ActionId.h"

namespace semantic {

class CapabilityFilter {
public:
    /**
     * @brief Filters a list of devices, keeping only those that support the required capabilities for a given intent.
     * @param candidates The list of candidate devices (e.g., returned from DeviceMatcher).
     * @param intent The canonical intent to be executed.
     * @return The filtered list of devices that can fulfill the intent.
     */
    std::vector<NetDiscovery::LogicalDevice> Filter(const std::vector<NetDiscovery::LogicalDevice>& candidates, 
                                                    NetDiscovery::ActionId intent) const;
};

} // namespace semantic
