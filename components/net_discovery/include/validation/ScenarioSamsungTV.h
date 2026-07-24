/**
 * @file ScenarioSamsungTV.h
 * @brief End-to-end acceptance validation scenario for Samsung TV UPnP action (v5.0.0 Architecture Phase 19).
 */

#pragma once

#include "validation/IValidationScenario.h"
#include "validation/ValidationReporter.h"
#include "discovery/DiscoveryManager.h"
#include "discovery/SSDPDiscoveryProvider.h"
#include "identity/IdentityManager.h"
#include "protocol/upnp/UPnPAdapter.h"
#include "testing/RuntimeTestHarness.h"

namespace NetDiscovery {
namespace Validation {

class ScenarioSamsungTV : public IValidationScenario {
public:
    ScenarioSamsungTV() = default;

    std::string GetScenarioName() const override { return "SamsungTV"; }
    ValidationReport Execute() override;
    bool Verify(const ValidationReport& report) const override;
    void PrintReport(const ValidationReport& report) const override;

private:
    ValidationReporter m_reporter;
};

} // namespace Validation
} // namespace NetDiscovery
