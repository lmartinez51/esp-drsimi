/**
 * @file AdapterResolver.cpp
 * @brief Implementation of AdapterResolver (v5.0.0 Architecture Phase 10.1).
 */

#include "protocol/AdapterResolver.h"

namespace NetDiscovery {
namespace Protocol {

AdapterResolver::AdapterResolver(ProtocolAdapterRegistry*          adapterRegistry,
                                 AdapterLifecycleManager*          lifecycleManager,
                                 Binding::ProtocolBindingRegistry* bindingRegistry,
                                 std::shared_ptr<Runtime::IExecutionClock> clock,
                                 StorageEventBus*                  eventBus)
    : m_adapterRegistry(adapterRegistry)
    , m_lifecycleManager(lifecycleManager)
    , m_bindingRegistry(bindingRegistry)
    , m_clock(std::move(clock))
    , m_eventBus(eventBus) {

    if (!m_clock) {
        m_clock = std::make_shared<Runtime::SystemExecutionClock>();
    }
}

void AdapterResolver::SetEventBus(StorageEventBus* eventBus) {
    m_eventBus = eventBus;
}

void AdapterResolver::PublishResolutionEvent(StorageEventType type,
                                             const AdapterId& adapterId,
                                             const std::string& stepId,
                                             const std::string& detail) {
    if (!m_eventBus) return;
    StorageEvent event;
    event.type     = type;
    event.entityId = adapterId;
    event.metadata["adapterId"] = adapterId;
    event.metadata["stepId"]    = stepId;
    if (!detail.empty()) {
        event.metadata["detail"] = detail;
    }
    m_eventBus->Publish(event);
}

AdapterResolutionResult AdapterResolver::Resolve(
        const Execution::ExecutionStep&        step,
        const Runtime::ExecutionRuntimeContext& context) {

    // 1. Determine adapterId from step or binding registry
    std::string targetAdapterId = step.GetAdapterId();

    if (targetAdapterId.empty() && m_bindingRegistry && !step.GetBindingId().empty()) {
        auto bindingOpt = m_bindingRegistry->FindBinding(step.GetBindingId());
        if (bindingOpt.has_value()) {
            targetAdapterId = bindingOpt->GetAdapterId();
        }
    }

    if (targetAdapterId.empty()) {
        PublishResolutionEvent(StorageEventType::AdapterResolutionFailed, "", step.GetStepId(), "AdapterId empty");
        return AdapterResolutionResult(
            ResolutionStatus::AdapterNotFound, std::nullopt,
            {"stepId=" + step.GetStepId()},
            "AdapterResolver: adapterId is empty in step and binding",
            {{"stepId", step.GetStepId()}});
    }

    // 2. Query registry (Lock-minimal: copy shared_ptr and descriptor under lock)
    if (!m_adapterRegistry) {
        PublishResolutionEvent(StorageEventType::AdapterResolutionFailed, targetAdapterId, step.GetStepId(), "Registry null");
        return AdapterResolutionResult(
            ResolutionStatus::AdapterNotFound, std::nullopt,
            {"stepId=" + step.GetStepId(), "adapterId=" + targetAdapterId},
            "AdapterResolver: ProtocolAdapterRegistry is null",
            {{"adapterId", targetAdapterId}});
    }

    std::shared_ptr<IProtocolAdapter> adapter = m_adapterRegistry->Find(targetAdapterId);

    if (!adapter) {
        PublishResolutionEvent(StorageEventType::AdapterResolutionFailed, targetAdapterId, step.GetStepId(), "Adapter not found");
        return AdapterResolutionResult(
            ResolutionStatus::AdapterNotFound, std::nullopt,
            {"stepId=" + step.GetStepId(), "adapterId=" + targetAdapterId},
            "AdapterResolver: no adapter registered for id='" + targetAdapterId + "'",
            {{"adapterId", targetAdapterId}});
    }

    // Copy descriptor and capabilities lock-free from adapter instance
    ProtocolAdapterDescriptor desc = adapter->GetDescriptor();
    Runtime::DispatcherCapabilities caps = adapter->GetCapabilities();

    // 3. Query state from LifecycleManager (if available)
    ProtocolAdapterState state(targetAdapterId);
    if (m_lifecycleManager) {
        auto stateOpt = m_lifecycleManager->GetAdapterState(targetAdapterId);
        if (stateOpt.has_value()) {
            state = stateOpt.value();
        }
    }

    // 4. Validate Lifecycle State
    if (state.lifecycleState == AdapterLifecycleState::Initializing) {
        PublishResolutionEvent(StorageEventType::AdapterResolutionFailed, targetAdapterId, step.GetStepId(), "Initializing");
        return AdapterResolutionResult(
            ResolutionStatus::Initializing, std::nullopt,
            {"stepId=" + step.GetStepId(), "adapterId=" + targetAdapterId},
            "AdapterResolver: adapter '" + targetAdapterId + "' is currently initializing",
            {{"adapterId", targetAdapterId}, {"state", ToString(state.lifecycleState)}});
    }

    if (state.lifecycleState == AdapterLifecycleState::Busy) {
        PublishResolutionEvent(StorageEventType::AdapterResolutionFailed, targetAdapterId, step.GetStepId(), "Busy");
        return AdapterResolutionResult(
            ResolutionStatus::Busy, std::nullopt,
            {"stepId=" + step.GetStepId(), "adapterId=" + targetAdapterId},
            "AdapterResolver: adapter '" + targetAdapterId + "' is currently busy",
            {{"adapterId", targetAdapterId}, {"state", ToString(state.lifecycleState)}});
    }

    if (state.lifecycleState == AdapterLifecycleState::Error) {
        PublishResolutionEvent(StorageEventType::AdapterResolutionFailed, targetAdapterId, step.GetStepId(), "LifecycleError");
        return AdapterResolutionResult(
            ResolutionStatus::LifecycleError, std::nullopt,
            {"stepId=" + step.GetStepId(), "adapterId=" + targetAdapterId},
            "AdapterResolver: adapter '" + targetAdapterId + "' is in error state (" + state.lastError + ")",
            {{"adapterId", targetAdapterId}, {"error", state.lastError}});
    }

    // 5. Validate Availability
    bool available = adapter->IsAvailable();
    if (!available) {
        PublishResolutionEvent(StorageEventType::AdapterUnavailable, targetAdapterId, step.GetStepId(), "Unavailable");
        return AdapterResolutionResult(
            ResolutionStatus::Unavailable, std::nullopt,
            {"stepId=" + step.GetStepId(), "adapterId=" + targetAdapterId},
            "AdapterResolver: adapter '" + targetAdapterId + "' reports unavailable",
            {{"adapterId", targetAdapterId}});
    }

    // 6. Validate Capabilities & Operation Support
    if (!step.GetOperationId().empty() && !desc.supportedOperations.empty()) {
        if (!desc.SupportsOperation(step.GetOperationId())) {
            PublishResolutionEvent(StorageEventType::CapabilityMismatch, targetAdapterId, step.GetStepId(), "Operation not supported");
            return AdapterResolutionResult(
                ResolutionStatus::CapabilityMismatch, std::nullopt,
                {"stepId=" + step.GetStepId(), "operationId=" + step.GetOperationId()},
                "AdapterResolver: adapter '" + targetAdapterId + "' does not support operation '" + step.GetOperationId() + "'",
                {{"adapterId", targetAdapterId}, {"operationId", step.GetOperationId()}});
        }
    }

    // 7. Successful resolution
    uint64_t now = m_clock->NowMs();
    ResolvedAdapter resolved(targetAdapterId, adapter, desc, caps, state, now, state.stateVersion);

    PublishResolutionEvent(StorageEventType::AdapterResolved, targetAdapterId, step.GetStepId(), "Resolved successfully");

    return AdapterResolutionResult(
        ResolutionStatus::Resolved,
        resolved,
        {"stepId=" + step.GetStepId(), "adapterId=" + targetAdapterId},
        "",
        {{"adapterId", targetAdapterId}, {"protocol", desc.protocolName}});
}

AdapterResolutionResult AdapterResolver::Resolve(
        const Execution::ExecutionStep&        step,
        const Binding::ActionBinding&          binding,
        const Runtime::ExecutionRuntimeContext& context) {
    // If step adapterId is empty, construct a synthetic step with binding's adapterId
    if (step.GetAdapterId().empty()) {
        Execution::ExecutionStep boundStep(
            step.GetStepId(),
            binding.GetBindingId(),
            binding.GetAdapterId(),
            binding.GetOperationId().empty() ? step.GetOperationId() : binding.GetOperationId(),
            step.GetParameterValues(),
            step.GetEstimatedDurationMs(),
            step.GetTimeoutMs(),
            step.GetRollbackStepId(),
            step.IsOptional(),
            step.GetParallelGroup(),
            step.GetMetadata());
        return Resolve(boundStep, context);
    }
    return Resolve(step, context);
}

} // namespace Protocol
} // namespace NetDiscovery
