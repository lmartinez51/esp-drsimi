/**
 * @file PlanValidationResult.h
 * @brief Validation result model returned by PlanValidator (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/PlanningDiagnostic.h"

#include <vector>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Comprehensive validation outcome containing status, errors, warnings, and diagnostics.
 */
struct PlanValidationResult {
    bool valid{true};                                 // True if plan passes validation checks
    std::vector<PlanningDiagnostic> errors;           // Fatal validation errors
    std::vector<PlanningDiagnostic> warnings;         // Non-fatal optimization warnings
    std::vector<PlanningDiagnostic> diagnostics;      // Structural diagnostic logs

    PlanValidationResult() = default;

    void AddDiagnostic(PlanningDiagnostic diag) {
        if (diag.severity == DiagnosticSeverity::Error) {
            valid = false;
            errors.push_back(diag);
        } else if (diag.severity == DiagnosticSeverity::Warning) {
            warnings.push_back(diag);
        } else {
            diagnostics.push_back(diag);
        }
    }
};

} // namespace Execution
} // namespace NetDiscovery
