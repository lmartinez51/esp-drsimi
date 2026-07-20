/**
 * @file ResolutionResult.h
 * @brief The final result of a controller evaluation for a device.
 */

#pragma once

#include "ControllerCandidate.h"
#include "ResolutionDiagnostics.h"
#include "../IDeviceController.h"
#include <vector>

namespace NetDiscovery {

/**
 * @brief Outcome of evaluating a device through the Controller Resolver Engine.
 */
struct ResolutionResult {
    /// The best matching controller (can be nullptr if no match).
    const IDeviceController* winner{nullptr};
    
    /// The second best matching controller (can be nullptr).
    const IDeviceController* runnerUp{nullptr};
    
    /// All evaluated candidates.
    std::vector<ControllerCandidate> candidates;
    
    /// Diagnostics for the winning controller.
    ResolutionDiagnostics diagnostics;
    
    /// Final confidence score for the winning controller (0-100).
    int confidence{0};
};

} // namespace NetDiscovery
