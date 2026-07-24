/**
 * @file ProtocolAdapterDispatcher.h
 * @brief Execution dispatcher delegating adapter resolution to IAdapterResolver (v5.0.0 Architecture Phase 10.1).
 */

#pragma once

#include "runtime/IExecutionDispatcher.h"
#include "protocol/IAdapterResolver.h"
#include "protocol/ResolvedAdapter.h"

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Bridges the frozen IExecutionDispatcher interface to IAdapterResolver and IProtocolAdapter.
 *
 * ProtocolAdapterDispatcher performs ZERO registry lookups, ZERO factory queries, and ZERO adapter instantiations.
 * It uses m_resolver->Resolve() (or a pre-resolved ResolvedAdapter) and calls adapter->Execute().
 */
class ProtocolAdapterDispatcher : public Runtime::IExecutionDispatcher {
public:
    explicit ProtocolAdapterDispatcher(IAdapterResolver* resolver = nullptr);

    ~ProtocolAdapterDispatcher() override = default;

    void SetResolver(IAdapterResolver* resolver);

    // ── IExecutionDispatcher ────────────────────────────────────────────────

    Runtime::ExecutionStepResult Dispatch(
        const Execution::ExecutionStep&        step,
        Execution::ExecutionSession&           session,
        Runtime::ExecutionRuntimeContext&      context) override;

    Runtime::DispatcherCapabilities GetCapabilities() const override;

    // ── Pre-resolved Execution Helper ───────────────────────────────────────

    /**
     * @brief Directly dispatches a step using an already-resolved ResolvedAdapter.
     */
    Runtime::ExecutionStepResult DispatchResolved(
        const ResolvedAdapter&                 resolved,
        const Execution::ExecutionStep&        step,
        Execution::ExecutionSession&           session,
        Runtime::ExecutionRuntimeContext&      context);

private:
    IAdapterResolver* m_resolver{nullptr};
};

} // namespace Protocol
} // namespace NetDiscovery
