/**
 * @file DefaultRuntimeDiagnostics.cpp
 * @brief Implementation of DefaultRuntimeDiagnostics (v6.0 Phase C.5).
 */

#include "plan/DefaultRuntimeDiagnostics.h"
#include "esp_log.h"

static const char* TAG = "RuntimeDiagnostics";

namespace NetDiscovery {
namespace Plan {

void DefaultRuntimeDiagnostics::OnExecutionEvent(const ExecutionEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);

    switch (event.type) {
        case ExecutionEventType::PlanStarted:
            m_stats.totalPlansExecuted++;
            ESP_LOGI(TAG, "Plan Started: %s (Instance: %s)", event.planId.c_str(), event.instanceId.c_str());
            break;

        case ExecutionEventType::PlanFinished:
            m_stats.totalPlansSucceeded++;
            m_totalDurationSumMs += event.progress.elapsedTimeMs;
            if (m_stats.totalPlansSucceeded > 0) {
                m_stats.averageDurationMs = m_totalDurationSumMs / m_stats.totalPlansSucceeded;
            }
            ESP_LOGI(TAG, "Plan Finished: %s in %llu ms", event.planId.c_str(), static_cast<unsigned long long>(event.progress.elapsedTimeMs));
            break;

        case ExecutionEventType::PlanFailed:
            m_stats.totalPlansFailed++;
            ESP_LOGE(TAG, "Plan Failed: %s Reason: %s", event.planId.c_str(), event.errorMessage.c_str());
            break;

        case ExecutionEventType::Cancelled:
            m_stats.totalPlansCancelled++;
            ESP_LOGW(TAG, "Plan Cancelled: %s", event.planId.c_str());
            break;

        case ExecutionEventType::RollbackStarted:
            m_stats.totalRollbacksAttempted++;
            ESP_LOGW(TAG, "Rollback Started for Plan: %s", event.planId.c_str());
            break;

        default:
            break;
    }
}

PlanStatistics DefaultRuntimeDiagnostics::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void DefaultRuntimeDiagnostics::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats = PlanStatistics();
    m_totalDurationSumMs = 0;
}

} // namespace Plan
} // namespace NetDiscovery
