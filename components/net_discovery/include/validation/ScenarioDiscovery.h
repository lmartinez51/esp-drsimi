/**
 * @file ScenarioDiscovery.h
 * @brief Validation scenario for Discovery Framework (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/IValidationScenario.h"
#include "validation/ValidationReporter.h"
#include "discovery/DiscoveryManager.h"
#include "discovery/SSDPDiscoveryProvider.h"

namespace NetDiscovery {
namespace Validation {

class ScenarioDiscovery : public IValidationScenario {
public:
    ScenarioDiscovery() = default;

    std::string GetScenarioName() const override { return "Discovery"; }
    ValidationReport Execute() override;
    bool Verify(const ValidationReport& report) const override;
    void PrintReport(const ValidationReport& report) const override;

private:
    ValidationReporter m_reporter;
};

} // namespace Validation
} // namespace NetDiscovery
