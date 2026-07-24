/**
 * @file ValidationRunner.cpp
 * @brief Implementation of ValidationRunner (v5.0.0 Architecture Phase 19).
 */

#include "validation/ValidationRunner.h"
#include "esp_log.h"

static const char* TAG = "VALIDATION_RUNNER";

namespace NetDiscovery {
namespace Validation {

ValidationRunner::ValidationRunner() = default;

void ValidationRunner::RegisterScenario(std::shared_ptr<IValidationScenario> scenario) {
    if (!scenario) return;
    std::string name = scenario->GetScenarioName();
    m_scenarios.insert_or_assign(name, std::move(scenario));
}

ValidationReport ValidationRunner::Run(const std::string& scenarioName) {
    auto it = m_scenarios.find(scenarioName);
    if (it == m_scenarios.end()) {
        ESP_LOGE(TAG, "Scenario not found: %s", scenarioName.c_str());
        return ValidationReport(scenarioName, false, 0, {"Scenario not registered"});
    }

    ValidationReport report = it->second->Execute();
    it->second->PrintReport(report);
    return report;
}

std::vector<ValidationReport> ValidationRunner::RunAll() {
    std::vector<ValidationReport> results;
    results.reserve(m_scenarios.size());

    for (const auto& [name, scenario] : m_scenarios) {
        if (scenario) {
            ValidationReport report = scenario->Execute();
            scenario->PrintReport(report);
            results.push_back(report);
        }
    }
    return results;
}

std::vector<std::string> ValidationRunner::GetRegisteredScenarioNames() const {
    std::vector<std::string> names;
    names.reserve(m_scenarios.size());
    for (const auto& [name, sc] : m_scenarios) {
        names.push_back(name);
    }
    return names;
}

} // namespace Validation
} // namespace NetDiscovery
