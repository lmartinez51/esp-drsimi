/**
 * @file ValidationReport.h
 * @brief Immutable validation report object for scenario acceptance results (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Validation {

/**
 * @brief Immutable report object containing scenario execution metrics and diagnostics.
 */
struct ValidationReport {
    std::string scenarioName;
    uint64_t    startTimestampMs{0};
    uint64_t    finishTimestampMs{0};
    uint32_t    durationMs{0};
    bool        passed{false};
    std::vector<std::string> diagnostics;
    std::vector<std::string> executionTrace;
    std::vector<std::string> warnings;

    ValidationReport() = default;

    ValidationReport(std::string name,
                     bool isPassed,
                     uint32_t duration,
                     std::vector<std::string> diag = {},
                     std::vector<std::string> trace = {},
                     std::vector<std::string> warn = {})
        : scenarioName(std::move(name))
        , durationMs(duration)
        , passed(isPassed)
        , diagnostics(std::move(diag))
        , executionTrace(std::move(trace))
        , warnings(std::move(warn)) {}
};

} // namespace Validation
} // namespace NetDiscovery
