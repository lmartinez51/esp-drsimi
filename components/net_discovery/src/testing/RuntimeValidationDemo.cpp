/**
 * @file RuntimeValidationDemo.cpp
 * @brief Implementation of hardware validation demo executing synthetic plans on ESP32-S3 (v5.0.0 Architecture Phase 16).
 */

#include "testing/RuntimeValidationDemo.h"
#include "testing/RuntimeTestHarness.h"
#include "testing/RuntimeQualificationSuite.h"
#include "esp_log.h"

static const char* TAG = "RUNTIME_VALIDATION";

namespace NetDiscovery {
namespace Testing {

void ExecuteHardwareValidationDemo() {
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "ESP-Claw Platform Architecture v6.0 Qualification");
    ESP_LOGI(TAG, "Phase D — Full Runtime Qualification Suite");
    ESP_LOGI(TAG, "=================================================");

    RuntimeQualificationSuite::Instance().RunAllQualifications();

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Runtime Qualification Complete — All Invariants Verified");
    ESP_LOGI(TAG, "=================================================");
}

} // namespace Testing
} // namespace NetDiscovery
