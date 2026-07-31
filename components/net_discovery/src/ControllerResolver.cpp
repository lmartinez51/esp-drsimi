/**
 * @file ControllerResolver.cpp
 * @brief ControllerResolver implementation.
 */

#include "../include/ControllerResolver.h"
#include <algorithm>

namespace NetDiscovery {

ControllerResolver::ControllerResolver(const ControllerRegistry& registry)
    : m_registry(registry)
{
}

void ControllerResolver::Resolve(LogicalDevice& device) const {
    struct EvaluatedCandidate {
        const IDeviceController* controller;
        ControllerCandidate candidate;
        ResolutionDiagnostics diag;
    };

    std::vector<EvaluatedCandidate> evaluated;

    for (const auto& controller : m_registry.GetControllers()) {
        ControllerCandidate candidate;
        candidate.name = controller->ControllerName();
        
        // Stage 1: Mandatory Requirements
        if (!controller->IsMatch(device)) {
            candidate.isRejected = true;
            candidate.diagnosticReason = "Failed IsMatch (Mandatory Requirements)";
            candidate.scoreBreakdown.clear();
            device.controllerCandidates.push_back(candidate);
            continue; // Rejected immediately
        }

        // Stage 2: Confidence Scoring
        ResolutionDiagnostics diag = controller->Evaluate(device);
        
        candidate.confidence = diag.score;
        candidate.diagnosticReason = diag.reason;
        candidate.scoreBreakdown = diag.scoreBreakdown;
        
        evaluated.push_back({controller.get(), candidate, diag});
    }

    // Sort by confidence descending
    std::sort(evaluated.begin(), evaluated.end(), [](const EvaluatedCandidate& a, const EvaluatedCandidate& b) {
        return a.candidate.confidence > b.candidate.confidence;
    });

    // We already added Stage 1 rejected ones, now add Stage 3 validated/rejected ones
    bool winnerSelected = false;
    
    for (auto& ev : evaluated) {
        if (!winnerSelected) {
            if (ev.controller->ValidateEndpoints(device)) {
                // This is the winner
                device.controllerCandidates.push_back(ev.candidate); // Winner goes first
                
                // Add vendor actions to the device
                auto vendorActions = ev.controller->VendorActions();
                for (const auto& va : vendorActions) {
                    // Check if already exists from generic ActionResolver
                    bool exists = false;
                    for (const auto& existingAction : device.actions) {
                        if (existingAction.id == va.id) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        device.actions.push_back(va);
                    }
                }
                
                winnerSelected = true;
            } else {
                ev.candidate.isRejected = true;
                ev.candidate.diagnosticReason = "Failed ValidateEndpoints";
                device.controllerCandidates.push_back(ev.candidate);
            }
        } else {
            // Already have a winner, these are valid alternatives
            if (ev.controller->ValidateEndpoints(device)) {
                device.controllerCandidates.push_back(ev.candidate);
            } else {
                ev.candidate.isRejected = true;
                ev.candidate.diagnosticReason = "Failed ValidateEndpoints";
                device.controllerCandidates.push_back(ev.candidate);
            }
        }
    }
    
    // Ensure the winner (if any) is at index 0 by partitioning valid vs rejected, 
    // though the current logic pushes the winner first among the evaluated, 
    // but we already pushed Stage 1 rejects. Let's sort to put valid highest scores first.
    std::sort(device.controllerCandidates.begin(), device.controllerCandidates.end(), 
        [](const ControllerCandidate& a, const ControllerCandidate& b) {
            if (a.isRejected != b.isRejected) return b.isRejected; // false (not rejected) comes first
            return a.confidence > b.confidence;
        });
}

} // namespace NetDiscovery
