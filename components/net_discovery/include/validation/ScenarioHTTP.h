/**
 * @file ScenarioHTTP.h
 * @brief Validation scenario for REST HTTP Protocol Adapter (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/IValidationScenario.h"
#include "validation/ValidationReporter.h"
#include "protocol/http/HTTPAdapter.h"
#include "protocol/http/IHTTPTransport.h"

namespace NetDiscovery {
namespace Validation {

class ScenarioHTTP : public IValidationScenario {
public:
    ScenarioHTTP() = default;

    std::string GetScenarioName() const override { return "HTTP"; }
    ValidationReport Execute() override;
    bool Verify(const ValidationReport& report) const override;
    void PrintReport(const ValidationReport& report) const override;

private:
    ValidationReporter m_reporter;
};

} // namespace Validation
} // namespace NetDiscovery
