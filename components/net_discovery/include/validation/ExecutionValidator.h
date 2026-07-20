#pragma once

#include <vector>
#include <string>
#include "../core/LogicalDevice.h"
#include "../core/ActionId.h"
#include "../DeviceExecutor.h"

namespace NetDiscovery {
namespace validation {

/**
 * @brief Represents a programmatic test scenario for the Execution Framework.
 */
struct ExecutionScenario {
    std::string name;
    Capability requiredCapability;
    std::vector<ActionId> actionsToTest;
};

/**
 * @brief Subsystem dedicated to exercising the Execution Framework independently of any vendor.
 */
class ExecutionValidator {
public:
    ExecutionValidator(DeviceExecutor& executor);
    
    /**
     * @brief Executes a predefined scenario if a device with the required capability is found.
     */
    void RunScenario(const std::vector<LogicalDevice>& devices, const ExecutionScenario& scenario);

private:
    DeviceExecutor& m_executor;
};

} // namespace validation
} // namespace NetDiscovery
