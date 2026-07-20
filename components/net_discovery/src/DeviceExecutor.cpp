#include "DeviceExecutor.h"

namespace NetDiscovery {

DeviceExecutor::DeviceExecutor(const TransportRegistry& transportRegistry, 
                               const ControllerRegistry& controllerRegistry,
                               std::shared_ptr<AuthenticationManager> authManager)
    : engine(transportRegistry, controllerRegistry, authManager) {
}

ExecutionResult DeviceExecutor::Execute(const ExecutionRequest& request) {
    return engine.Execute(request);
}

} // namespace NetDiscovery
