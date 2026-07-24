/**
 * @file DefaultRuntimeDiagnostics.h
 * @brief Thread-safe observer implementation of IRuntimeDiagnostics (v6.0 Phase C.5).
 */

#pragma once

#include "plan/IRuntimeDiagnostics.h"
#include <mutex>

namespace NetDiscovery {
namespace Plan {

class DefaultRuntimeDiagnostics : public IRuntimeDiagnostics {
public:
    DefaultRuntimeDiagnostics() = default;

    void OnExecutionEvent(const ExecutionEvent& event) override;

    PlanStatistics GetStatistics() const override;
    void Reset() override;

private:
    PlanStatistics m_stats;
    uint64_t m_totalDurationSumMs{0};
    mutable std::mutex m_mutex;
};

} // namespace Plan
} // namespace NetDiscovery
