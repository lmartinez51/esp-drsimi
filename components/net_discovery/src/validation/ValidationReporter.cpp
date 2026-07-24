/**
 * @file ValidationReporter.cpp
 * @brief Implementation of ValidationReporter (v5.0.0 Architecture Phase 19).
 */

#include "validation/ValidationReporter.h"
#include "esp_log.h"

static const char* TAG = "VALIDATION_REPORTER";

namespace NetDiscovery {
namespace Validation {

void ValidationReporter::PrintSummary(const ValidationReport& report) const {
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Scenario: %s", report.scenarioName.c_str());
    ESP_LOGI(TAG, "==========================================");

    for (const auto& diag : report.diagnostics) {
        ESP_LOGI(TAG, "  %s", diag.c_str());
    }

    ESP_LOGI(TAG, "------------------------------------------");
    ESP_LOGI(TAG, "Outcome ..... %s", report.passed ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "Total Time .. %lu ms", static_cast<unsigned long>(report.durationMs));
    ESP_LOGI(TAG, "==========================================");
}

void ValidationReporter::PrintTrace(const ValidationReport& report) const {
    ESP_LOGI(TAG, "--- Execution Trace: %s ---", report.scenarioName.c_str());
    for (const auto& line : report.executionTrace) {
        ESP_LOGI(TAG, "  %s", line.c_str());
    }
}

} // namespace Validation
} // namespace NetDiscovery
