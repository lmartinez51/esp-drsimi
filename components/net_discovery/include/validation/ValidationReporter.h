/**
 * @file ValidationReporter.h
 * @brief Report formatter generating standardized console log outputs (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/ValidationReport.h"
#include <string>

namespace NetDiscovery {
namespace Validation {

/**
 * @brief Responsible exclusively for formatting and printing scenario execution reports to ESP_LOG.
 */
class ValidationReporter {
public:
    ValidationReporter() = default;

    void PrintSummary(const ValidationReport& report) const;
    void PrintTrace(const ValidationReport& report) const;
};

} // namespace Validation
} // namespace NetDiscovery
