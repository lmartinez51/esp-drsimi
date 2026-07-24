/**
 * @file StepMetadata.h
 * @brief Immutable step annotation metadata (v6.0 Phase D).
 */

#pragma once

#include <string>
#include <vector>

namespace NetDiscovery {
namespace Plan {

struct StepMetadata {
    std::string stepId;
    std::string stepName;
    std::string description;
    std::string author;
    std::vector<std::string> tags;
    std::string version{"1.0.0"};
};

} // namespace Plan
} // namespace NetDiscovery
