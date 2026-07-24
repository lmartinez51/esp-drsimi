/**
 * @file ScenarioIdentity.h
 * @brief Validation scenario for Identity Layer (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/IValidationScenario.h"
#include "validation/ValidationReporter.h"
#include "identity/IdentityManager.h"

namespace NetDiscovery {
namespace Validation {

class ScenarioIdentity : public IValidationScenario {
public:
    ScenarioIdentity() = default;

    std::string GetScenarioName() const override { return "Identity"; }
    ValidationReport Execute() override;
    bool Verify(const ValidationReport& report) const override;
    void PrintReport(const ValidationReport& report) const override;

private:
    ValidationReporter m_reporter;
};

} // namespace Validation
} // namespace NetDiscovery
