/**
 * @file SyntheticPlanBuilder.h
 * @brief Helper class constructing synthetic ExecutionPlan instances for end-to-end testing (v5.0.0 Architecture Phase 16).
 */

#pragma once

#include "execution/ExecutionPlan.h"
#include "execution/ExecutionStep.h"
#include "protocol/capability/ProtocolCapabilityRequirement.h"

namespace NetDiscovery {
namespace Testing {

/**
 * @brief Helper builder constructing test ExecutionPlan instances.
 */
class SyntheticPlanBuilder {
public:
    static Execution::ExecutionPlan BuildSingleStepPlan(
        const std::string& stepId = "step_1",
        const std::string& operationId = "GET",
        const std::string& adapterId = "adapter.http.default",
        Protocol::ProtocolCapabilityRequirement requirement = {}) {

        Execution::ExecutionStep step(
            stepId, "binding_1", adapterId, operationId,
            {{"path", "/api/test"}}, 100, 5000, std::nullopt, false, 0, {}, requirement);

        return Execution::ExecutionPlan("plan_synthetic_single", "req_1", {step});
    }

    static Execution::ExecutionPlan BuildMultiStepSequentialPlan() {
        Execution::ExecutionStep step1("step_1", "binding_1", "adapter.http.default", "GET", {{"path", "/api/status"}});
        Execution::ExecutionStep step2("step_2", "binding_2", "adapter.http.default", "POST", {{"path", "/api/command"}, {"body", "{\"cmd\":\"on\"}"}});

        return Execution::ExecutionPlan("plan_synthetic_multi", "req_2", {step1, step2}, {{"step_2", {"step_1"}}});
    }
};

} // namespace Testing
} // namespace NetDiscovery
