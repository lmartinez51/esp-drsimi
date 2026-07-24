/**
 * @file ValidationReport.h
 * @brief Aggregated report containing all validation issues detected during a single-pass verification (v6.0 Phase C.5).
 */

#pragma once

#include "plan/ValidationIssue.h"
#include <vector>
#include <string>

namespace NetDiscovery {
namespace Plan {

class ValidationReport {
public:
    ValidationReport() = default;

    void AddIssue(ValidationIssue issue);
    void AddIssue(ValidationSeverity severity, ValidationCode code, std::string componentId, std::string message);

    bool IsValid() const;
    bool HasErrors() const;
    bool HasWarnings() const;

    const std::vector<ValidationIssue>& GetIssues() const { return m_issues; }
    std::string ToString() const;

private:
    std::vector<ValidationIssue> m_issues;
};

} // namespace Plan
} // namespace NetDiscovery
