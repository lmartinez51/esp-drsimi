/**
 * @file ValidationReport.cpp
 * @brief Implementation of ValidationReport (v6.0 Phase C.5).
 */

#include "plan/ValidationReport.h"

namespace NetDiscovery {
namespace Plan {

void ValidationReport::AddIssue(ValidationIssue issue) {
    m_issues.push_back(std::move(issue));
}

void ValidationReport::AddIssue(ValidationSeverity severity, ValidationCode code, std::string componentId, std::string message) {
    ValidationIssue issue;
    issue.severity = severity;
    issue.code = code;
    issue.componentId = std::move(componentId);
    issue.message = std::move(message);
    m_issues.push_back(std::move(issue));
}

bool ValidationReport::IsValid() const {
    return !HasErrors();
}

bool ValidationReport::HasErrors() const {
    for (const auto& issue : m_issues) {
        if (issue.severity == ValidationSeverity::Error || issue.severity == ValidationSeverity::Fatal) {
            return true;
        }
    }
    return false;
}

bool ValidationReport::HasWarnings() const {
    for (const auto& issue : m_issues) {
        if (issue.severity == ValidationSeverity::Warning) {
            return true;
        }
    }
    return false;
}

std::string ValidationReport::ToString() const {
    if (m_issues.empty()) {
        return "ValidationReport: Clean (0 issues)";
    }
    std::string result = "ValidationReport (" + std::to_string(m_issues.size()) + " issues):\n";
    for (const auto& issue : m_issues) {
        result += "  - " + issue.ToString() + "\n";
    }
    return result;
}

} // namespace Plan
} // namespace NetDiscovery
