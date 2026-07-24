/**
 * @file ProtocolAdapterDispatcher.cpp
 * @brief Implementation of ProtocolAdapterDispatcher (v5.0.0 Architecture Phase 10.1).
 */

#include "protocol/ProtocolAdapterDispatcher.h"

#include "protocol/capability/ProtocolCapabilityValidator.h"

namespace NetDiscovery {
namespace Protocol {

ProtocolAdapterDispatcher::ProtocolAdapterDispatcher(IAdapterResolver* resolver)
    : m_resolver(resolver) {}

void ProtocolAdapterDispatcher::SetResolver(IAdapterResolver* resolver) {
    m_resolver = resolver;
}

Runtime::ExecutionStepResult ProtocolAdapterDispatcher::Dispatch(
        const Execution::ExecutionStep&        step,
        Execution::ExecutionSession&           session,
        Runtime::ExecutionRuntimeContext&      context) {

    if (!m_resolver) {
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::NotImplemented,
            step.GetAdapterId(), 0, 0, -1,
            "ProtocolAdapterDispatcher: IAdapterResolver not injected",
            false, false, {}, {},
            {{"phase", "10.1"}, {"reason", "no_resolver"}});
    }

    // 1. Resolve step using IAdapterResolver
    AdapterResolutionResult resResult = m_resolver->Resolve(step, context);

    if (!resResult.IsResolved() || !resResult.HasAdapter()) {
        Execution::StepStatus status = Execution::StepStatus::Failure;
        if (resResult.status == ResolutionStatus::AdapterNotFound ||
            resResult.status == ResolutionStatus::CapabilityMismatch) {
            status = Execution::StepStatus::NotImplemented;
        }

        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            status,
            step.GetAdapterId(), 0, 0, static_cast<int32_t>(resResult.status),
            "ProtocolAdapterDispatcher: resolution failed (" + ToString(resResult.status) + "): " + resResult.failureReason,
            (resResult.status == ResolutionStatus::Busy || resResult.status == ResolutionStatus::Initializing),
            false,
            resResult.diagnostics,
            {},
            resResult.metadata);
    }

    // 2. Dispatch using resolved adapter
    return DispatchResolved(resResult.resolvedAdapter.value(), step, session, context);
}

Runtime::ExecutionStepResult ProtocolAdapterDispatcher::DispatchResolved(
        const ResolvedAdapter&                 resolved,
        const Execution::ExecutionStep&        step,
        Execution::ExecutionSession&           session,
        Runtime::ExecutionRuntimeContext&      context) {

    if (!resolved.IsValid()) {
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::Failure,
            step.GetAdapterId(), 0, 0, -2,
            "ProtocolAdapterDispatcher: invalid ResolvedAdapter passed to DispatchResolved",
            false, false, {}, {},
            {{"adapterId", step.GetAdapterId()}});
    }

    // 3. Phase 13: Protocol Capability Validation before execution
    ProtocolCapabilityValidator validator;
    CapabilityValidationResult valResult = validator.Validate(
        step, step.GetCapabilityRequirement(), resolved.adapter->GetProtocolCapabilities());

    if (!valResult.IsValid()) {
        return Runtime::ExecutionStepResult(
            step.GetStepId(),
            Execution::StepStatus::Failure,
            step.GetAdapterId(), 0, 0, -3,
            "ProtocolAdapterDispatcher: capability validation failed (" + valResult.GetSummary() + ")",
            false, false, valResult.diagnostics, {},
            {{"reason", "capability_mismatch"}},
            Runtime::ExecutionFailureReason::CapabilityMismatch,
            "ProtocolAdapterDispatcher",
            -3,
            valResult.GetSummary());
    }

    // Call adapter->Execute ONLY after capability validation passes.
    return resolved.adapter->Execute(step, session, context);
}


Runtime::DispatcherCapabilities ProtocolAdapterDispatcher::GetCapabilities() const {
    return Runtime::DispatcherCapabilities::BasicSynchronous();
}

} // namespace Protocol
} // namespace NetDiscovery
