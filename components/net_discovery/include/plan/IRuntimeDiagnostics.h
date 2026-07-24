/**
 * @file IRuntimeDiagnostics.h
 * @brief Interface for inspecting runtime metrics, plan duration, and step telemetry (v6.0 Phase C.5).
 */

#pragma once

#include "plan/IExecutionPlanObserver.h"
#include <cstdint>
#include <string>

namespace NetDiscovery {
namespace Plan {

struct PlanStatistics {
    uint64_t totalPlansExecuted{0};
    uint64_t totalPlansSucceeded{0};
    uint64_t totalPlansFailed{0};
    uint64_t totalPlansCancelled{0};
    uint64_t totalRollbacksAttempted{0};
    uint64_t averageDurationMs{0};
};

class IRuntimeDiagnostics : public IExecutionPlanObserver {
public:
    virtual ~IRuntimeDiagnostics() = default;

    virtual PlanStatistics GetStatistics() const = 0;
    virtual void Reset() = 0;
};

} // namespace Plan
} // namespace NetDiscovery
