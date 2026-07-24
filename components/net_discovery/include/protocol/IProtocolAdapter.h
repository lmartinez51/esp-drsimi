/**
 * @file IProtocolAdapter.h
 * @brief Permanent base interface for all protocol adapter implementations (v5.0.0 Architecture Phase 13).
 */

#pragma once

#include "protocol/ProtocolAdapterDescriptor.h"
#include "protocol/ProtocolAdapterState.h"
#include "protocol/capability/ProtocolCapabilitySet.h"
#include "runtime/DispatcherCapabilities.h"
#include "runtime/ExecutionStepResult.h"
#include "runtime/ExecutionRuntimeContext.h"
#include "execution/ExecutionStep.h"
#include "execution/ExecutionSession.h"

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Permanent base interface every protocol adapter must implement.
 *
 * IProtocolAdapter is the exclusive abstraction between the execution engine and
 * physical protocol implementations.
 */
class IProtocolAdapter {
public:
    virtual ~IProtocolAdapter() = default;

    // ── Lifecycle ───────────────────────────────────────────────────────────

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsAvailable() const = 0;

    // ── Metadata ────────────────────────────────────────────────────────────

    virtual const ProtocolAdapterDescriptor& GetDescriptor() const = 0;

    /**
     * @brief Returns the runtime dispatcher execution capabilities this adapter exposes (timeouts, batching, cancellation).
     */
    virtual Runtime::DispatcherCapabilities GetCapabilities() const = 0;

    /**
     * @brief Returns the intrinsic protocol feature capabilities supported by this adapter (QoS1, SOAP, CASE, Notify, TLS).
     */
    virtual ProtocolCapabilitySet GetProtocolCapabilities() const = 0;

    // ── Execution ───────────────────────────────────────────────────────────

    virtual Runtime::ExecutionStepResult Execute(
        const Execution::ExecutionStep&  step,
        Execution::ExecutionSession&     session,
        Runtime::ExecutionRuntimeContext& context) = 0;
};

} // namespace Protocol
} // namespace NetDiscovery
