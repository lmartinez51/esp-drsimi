/**
 * @file ScenarioRuntimePipeline.cpp
 * @brief Implementation of ScenarioRuntimePipeline (v5.0.0 Architecture Phase 19).
 */

#include "validation/ScenarioRuntimePipeline.h"

namespace NetDiscovery {
namespace Validation {

ValidationReport ScenarioRuntimePipeline::Execute() {
    std::vector<std::string> diag;
    std::vector<std::string> trace;

    trace.push_back("Instantiating RuntimeTestHarness");
    Testing::RuntimeTestHarness harness;

    trace.push_back("Running synthetic single step execution plan");
    Testing::TestHarnessResult res = harness.RunSuccessfulExecutionScenario();

    diag.push_back("Runtime Pipeline Result: " + std::string(res.passed ? "PASS" : "FAIL"));
    diag.push_back("Session ID: " + res.sessionId);

    return ValidationReport("RuntimePipeline", res.passed, 25, diag, trace);
}

bool ScenarioRuntimePipeline::Verify(const ValidationReport& report) const {
    return report.passed;
}

void ScenarioRuntimePipeline::PrintReport(const ValidationReport& report) const {
    m_reporter.PrintSummary(report);
}

} // namespace Validation
} // namespace NetDiscovery
