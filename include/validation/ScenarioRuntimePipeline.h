/**
 * @file ScenarioRuntimePipeline.h
 * @brief Validation scenario for Runtime execution pipeline (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/IValidationScenario.h"
#include "validation/ValidationReporter.h"
#include "testing/RuntimeTestHarness.h"

namespace NetDiscovery {
namespace Validation {

class ScenarioRuntimePipeline : public IValidationScenario {
public:
    ScenarioRuntimePipeline() = default;

    std::string GetScenarioName() const override { return "RuntimePipeline"; }
    ValidationReport Execute() override;
    bool Verify(const ValidationReport& report) const override;
    void PrintReport(const ValidationReport& report) const override;

private:
    ValidationReporter m_reporter;
};

} // namespace Validation
} // namespace NetDiscovery
