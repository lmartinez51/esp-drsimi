#include "DeviceExecutor.h"
#include "core/BoundExecutionRequest.h"

namespace NetDiscovery {

DeviceExecutor::DeviceExecutor(const TransportRegistry& transportRegistry, 
                               const ControllerRegistry& controllerRegistry,
                               std::shared_ptr<AuthenticationManager> authManager)
    : engine(transportRegistry, controllerRegistry, authManager) {
}

ExecutionResult DeviceExecutor::Execute(const ExecutionRequest& request) {
    BoundExecutionRequest boundReq;
    boundReq.targetDevice = &request.device;
    boundReq.action = request.action;
    boundReq.parameters = request.parameters;
    boundReq.context = request.context;
    return engine.Execute(boundReq);
}

} // namespace NetDiscovery
