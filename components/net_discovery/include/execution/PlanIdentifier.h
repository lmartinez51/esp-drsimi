/**
 * @file PlanIdentifier.h
 * @brief Immutable structured identifier for ExecutionPlans (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Structured, versioned immutable plan identifier.
 */
class PlanIdentifier {
public:
    PlanIdentifier() = default;

    PlanIdentifier(RequestId requestId,
                   uint32_t planVersion = 1,
                   uint64_t creationTimestampMs = 0,
                   uint32_t plannerVersion = 1)
        : m_requestId(std::move(requestId)),
          m_planVersion(planVersion),
          m_creationTimestampMs(creationTimestampMs),
          m_plannerVersion(plannerVersion) {
        m_canonicalString = "plan." + m_requestId + ".v" + std::to_string(m_planVersion) + "." + std::to_string(m_creationTimestampMs);
    }

    const RequestId& GetRequestId() const { return m_requestId; }
    uint32_t GetPlanVersion() const { return m_planVersion; }
    uint64_t GetCreationTimestampMs() const { return m_creationTimestampMs; }
    uint32_t GetPlannerVersion() const { return m_plannerVersion; }
    const std::string& ToString() const { return m_canonicalString; }

    bool operator==(const PlanIdentifier& other) const {
        return m_canonicalString == other.m_canonicalString;
    }

private:
    RequestId m_requestId;
    uint32_t m_planVersion{1};
    uint64_t m_creationTimestampMs{0};
    uint32_t m_plannerVersion{1};
    std::string m_canonicalString;
};

} // namespace Execution
} // namespace NetDiscovery
