/**
 * @file ScenarioDiscovery.cpp
 * @brief Implementation of ScenarioDiscovery (v5.0.0 Architecture Phase 19).
 */

#include "validation/ScenarioDiscovery.h"

namespace NetDiscovery {
namespace Validation {

ValidationReport ScenarioDiscovery::Execute() {
    std::vector<std::string> diag;
    std::vector<std::string> trace;

    trace.push_back("Instantiating SSDPDiscoveryProvider and DiscoveryManager");
    auto ssdpProvider = std::make_shared<Discovery::SSDPDiscoveryProvider>();
    auto registry     = std::make_shared<Discovery::DeviceRegistry>();
    Discovery::DiscoveryManager discoveryMgr(registry);

    discoveryMgr.RegisterProvider(ssdpProvider);

    // Add synthetic SSDP device
    ssdpProvider->AddSyntheticDiscoveredDevice(
        Discovery::DiscoveredDeviceDescriptor("ssdp.uuid.samsungtv1", "Living Room TV", "Samsung", "UE55", "LivingRoom", {"192.168.1.105"}, {"UPnP"}, {"PowerControl", "LaunchApp"})
    );

    trace.push_back("Starting Discovery");
    bool startOk = discoveryMgr.StartAllDiscovery();
    diag.push_back("Discovery Start: " + std::string(startOk ? "PASS" : "FAIL"));

    std::size_t devCount = registry->GetCount();
    diag.push_back("Discovered Devices: " + std::to_string(devCount));

    auto samsungTVs = registry->GetDevicesByManufacturer("Samsung");
    diag.push_back("Samsung Devices Found: " + std::to_string(samsungTVs.size()));

    bool passed = startOk && devCount >= 1 && !samsungTVs.empty();

    return ValidationReport("Discovery", passed, 15, diag, trace);
}

bool ScenarioDiscovery::Verify(const ValidationReport& report) const {
    return report.passed;
}

void ScenarioDiscovery::PrintReport(const ValidationReport& report) const {
    m_reporter.PrintSummary(report);
}

} // namespace Validation
} // namespace NetDiscovery
