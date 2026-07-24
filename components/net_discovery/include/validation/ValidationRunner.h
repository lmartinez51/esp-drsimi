/**
 * @file ValidationRunner.h
 * @brief Reusable scenario runner executing individual or batch acceptance tests (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/IValidationScenario.h"
#include "validation/ValidationReporter.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace NetDiscovery {
namespace Validation {

/**
 * @brief Reusable runner registering, selecting, and executing validation scenarios.
 */
class ValidationRunner {
public:
    ValidationRunner();
    ~ValidationRunner() = default;

    void RegisterScenario(std::shared_ptr<IValidationScenario> scenario);

    ValidationReport Run(const std::string& scenarioName);
    std::vector<ValidationReport> RunAll();

    std::vector<std::string> GetRegisteredScenarioNames() const;

private:
    ValidationReporter m_reporter;
    std::unordered_map<std::string, std::shared_ptr<IValidationScenario>> m_scenarios;
};

} // namespace Validation
} // namespace NetDiscovery
