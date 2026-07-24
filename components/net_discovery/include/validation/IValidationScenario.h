/**
 * @file IValidationScenario.h
 * @brief Pure interface for validation scenarios (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/ValidationReport.h"
#include <string>

namespace NetDiscovery {
namespace Validation {

/**
 * @brief Pure abstract interface implemented by all validation scenarios.
 */
class IValidationScenario {
public:
    virtual ~IValidationScenario() = default;

    virtual std::string GetScenarioName() const = 0;
    virtual ValidationReport Execute() = 0;
    virtual bool Verify(const ValidationReport& report) const = 0;
    virtual void PrintReport(const ValidationReport& report) const = 0;
};

} // namespace Validation
} // namespace NetDiscovery
