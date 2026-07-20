/**
 * @file PresentationFormatter.h
 * @brief Formats logical devices into human-readable console output.
 */

#pragma once

#include "core/LogicalDevice.h"
#include <vector>
#include <string>

namespace NetDiscovery {

/**
 * @brief Replaces raw enum printing with formatted human-readable output.
 */
class PresentationFormatter {
public:
    static void PrintLogicalDevices(const std::vector<LogicalDevice>& devices);
    
private:
    static std::string CapabilityToString(Capability cap);
    static std::string DiscoverySourceToString(DiscoverySource source);
};

} // namespace NetDiscovery
