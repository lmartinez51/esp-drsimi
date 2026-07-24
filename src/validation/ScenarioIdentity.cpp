/**
 * @file ScenarioIdentity.cpp
 * @brief Implementation of ScenarioIdentity (v5.0.0 Architecture Phase 19).
 */

#include "validation/ScenarioIdentity.h"

namespace NetDiscovery {
namespace Validation {

ValidationReport ScenarioIdentity::Execute() {
    std::vector<std::string> diag;
    std::vector<std::string> trace;

    trace.push_back("Instantiating IdentityManager");
    Identity::IdentityManager identityMgr;

    trace.push_back("Creating logical identity for Living Room TV");
    auto created = identityMgr.CreateIdentity("Living Room TV", "TV", "Samsung", "UE55");
    if (!created.has_value()) {
        return ValidationReport("Identity", false, 5, {"Failed to create identity"});
    }

    std::string id = created->identity.identityId;
    diag.push_back("Identity Created: " + id);

    trace.push_back("Linking SSDP discovery ID to logical identity");
    bool linkOk = identityMgr.LinkDiscoveredDevice(id, "ssdp.uuid.samsungtv1");
    diag.push_back("Discovery Link: " + std::string(linkOk ? "PASS" : "FAIL"));

    trace.push_back("Assigning room and registering alias");
    identityMgr.AssignRoom(id, "Living Room");
    identityMgr.AddAlias(id, "tv");

    auto resolved = identityMgr.ResolveIdentity("ssdp.uuid.samsungtv1");
    bool resolveOk = resolved.has_value() && resolved->identity.identityId == id;
    diag.push_back("Identity Resolution: " + std::string(resolveOk ? "PASS" : "FAIL"));

    bool passed = created.has_value() && linkOk && resolveOk;

    return ValidationReport("Identity", passed, 12, diag, trace);
}

bool ScenarioIdentity::Verify(const ValidationReport& report) const {
    return report.passed;
}

void ScenarioIdentity::PrintReport(const ValidationReport& report) const {
    m_reporter.PrintSummary(report);
}

} // namespace Validation
} // namespace NetDiscovery
