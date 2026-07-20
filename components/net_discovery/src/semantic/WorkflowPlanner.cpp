#include "semantic/WorkflowPlanner.h"

namespace semantic {

WorkflowTemplate WorkflowPlanner::GetTemplateForIntent(NetDiscovery::ActionId intent) const {
    WorkflowTemplate tmpl;
    tmpl.intent = intent;
    
    switch (intent) {
        case NetDiscovery::ActionId::LaunchApplication:
            tmpl.steps.push_back({NetDiscovery::ActionId::PowerOn, true, ExecutionWaitPolicy::WaitUntilReachable, std::chrono::milliseconds(5000), {3, std::chrono::milliseconds(1000)}});
            tmpl.steps.push_back({NetDiscovery::ActionId::LaunchApplication, false, ExecutionWaitPolicy::None, std::chrono::milliseconds(0), {1, std::chrono::milliseconds(0)}});
            break;
            
        case NetDiscovery::ActionId::PowerOn:
            tmpl.steps.push_back({NetDiscovery::ActionId::PowerOn, false, ExecutionWaitPolicy::WaitUntilReachable, std::chrono::milliseconds(5000), {3, std::chrono::milliseconds(1000)}});
            break;

        case NetDiscovery::ActionId::VolumeUp:
            tmpl.steps.push_back({NetDiscovery::ActionId::VolumeUp, false, ExecutionWaitPolicy::None, std::chrono::milliseconds(0), {1, std::chrono::milliseconds(0)}});
            break;

        case NetDiscovery::ActionId::VolumeDown:
            tmpl.steps.push_back({NetDiscovery::ActionId::VolumeDown, false, ExecutionWaitPolicy::None, std::chrono::milliseconds(0), {1, std::chrono::milliseconds(0)}});
            break;

        case NetDiscovery::ActionId::SetVolume:
            tmpl.steps.push_back({NetDiscovery::ActionId::SetVolume, false, ExecutionWaitPolicy::None, std::chrono::milliseconds(0), {1, std::chrono::milliseconds(0)}});
            break;

        case NetDiscovery::ActionId::Mute:
            tmpl.steps.push_back({NetDiscovery::ActionId::Mute, false, ExecutionWaitPolicy::None, std::chrono::milliseconds(0), {1, std::chrono::milliseconds(0)}});
            break;

        case NetDiscovery::ActionId::Unmute:
            tmpl.steps.push_back({NetDiscovery::ActionId::Unmute, false, ExecutionWaitPolicy::None, std::chrono::milliseconds(0), {1, std::chrono::milliseconds(0)}});
            break;

        default:
            // Direct map fallback
            tmpl.steps.push_back({intent, false, ExecutionWaitPolicy::None, std::chrono::milliseconds(0), {1, std::chrono::milliseconds(0)}});
            break;
    }
    
    return tmpl;
}

ExecutionPlan WorkflowPlanner::CreatePlan(NetDiscovery::ActionId intent, 
                                          const NetDiscovery::LogicalDevice& target,
                                          const std::map<std::string, ExecutionParameterVariant>& parameters) const {
    ExecutionPlan plan;
    plan.targets.push_back(target);

    WorkflowTemplate tmpl = GetTemplateForIntent(intent);
    
    for (const auto& tStep : tmpl.steps) {
        ExecutionStep step;
        step.stepId = NetDiscovery::ToString(tmpl.intent) + "_" + NetDiscovery::ToString(tStep.actionId);
        step.action.id = tStep.actionId;
        step.isOptional = tStep.isOptional;
        step.waitPolicy = tStep.waitPolicy;
        step.waitTimeout = tStep.waitTimeout;
        step.retryPolicy = tStep.retryPolicy;
        
        // Pass relevant parameters down
        for (const auto& [k, v] : parameters) {
            NetDiscovery::ActionParameter p;
            p.name = k;
            
            if (std::holds_alternative<int>(v)) p.type = "int";
            else if (std::holds_alternative<double>(v)) p.type = "double";
            else if (std::holds_alternative<bool>(v)) p.type = "bool";
            else if (std::holds_alternative<std::string>(v)) p.type = "string";

            step.action.supportedParameters.push_back(p);
        }

        plan.steps.push_back(step);
    }

    return plan;
}

} // namespace semantic
