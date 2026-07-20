#include "../include/validation/ExecutionValidator.h"
#include <iostream>

namespace NetDiscovery {
namespace validation {

ExecutionValidator::ExecutionValidator(DeviceExecutor& executor) 
    : m_executor(executor) {}

void ExecutionValidator::RunScenario(const std::vector<LogicalDevice>& devices, const ExecutionScenario& scenario) {
    std::cout << "======================================================================\n";
    std::cout << "  EXECUTION VALIDATION: " << scenario.name << "\n";
    std::cout << "======================================================================\n";

    const LogicalDevice* targetDevice = nullptr;
    for (const auto& dev : devices) {
        if (dev.HasCapability(scenario.requiredCapability)) {
            targetDevice = &dev;
            break;
        }
    }
    
    if (!targetDevice) {
        std::cout << "  [DEMO FAILED] No device exposing capability '" 
                  << ToString(scenario.requiredCapability) << "' was discovered.\n\n";
        return;
    }
    
    std::cout << "  [DEMO] Found capable device: " << targetDevice->displayName 
              << " (Supports " << ToString(scenario.requiredCapability) << ")\n\n";
    
    for (const auto& actionId : scenario.actionsToTest) {
        std::string actionStr = ToString(actionId);
        std::cout << "  --- Executing " << actionStr << " ---\n";
        ActionDescriptor actionToExecute;
        actionToExecute.id = actionId;
        actionToExecute.displayName = actionStr;

        std::map<std::string, std::string> reqParams;
        if (actionId == ActionId::LaunchApplication) {
            reqParams["name"] = "YouTube";
        }
        ExecutionRequest req { *targetDevice, actionToExecute, reqParams, ExecutionContext{}, 5000, 0 };
        ExecutionResult res = m_executor.Execute(req);
        
        std::cout << "  Execution Result: " << ToString(res.status) 
                  << " -- executed in " << res.elapsedTimeMs << "ms.\n";
                  
        if (!res.transportDiagnostics.rawPayload.empty()) {
            std::cout << "  Diagnostic Info : " << res.transportDiagnostics.rawPayload << "\n";
        }
        if (!res.parsedFields.empty()) {
            std::cout << "  Parsed Fields:\n";
            for (const auto& [k, v] : res.parsedFields) {
                std::cout << "    " << k << " = " << v << "\n";
            }
        }
        if (res.parserDiagnostics.hasError) {
            std::cout << "  Parser Error: " << res.parserDiagnostics.errorMessage << "\n";
        }
        std::cout << "\n";
    }
}

} // namespace validation
} // namespace NetDiscovery
