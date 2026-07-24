/**
 * @file AdapterResolver.h
 * @brief Concrete implementation of IAdapterResolver (v5.0.0 Architecture Phase 10.1).
 */

#pragma once

#include "protocol/IAdapterResolver.h"
#include "protocol/ProtocolAdapterRegistry.h"
#include "protocol/AdapterLifecycleManager.h"
#include "binding/ProtocolBindingRegistry.h"
#include "runtime/ExecutionClock.h"
#include "core/StorageEventBus.h"

#include <memory>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Deterministic, thread-safe resolver mapping ExecutionStep/ActionBinding to ResolvedAdapter.
 *
 * Minimal lock duration: acquiring registry locks only while copying shared_ptr<IProtocolAdapter>,
 * ProtocolAdapterDescriptor, and ProtocolAdapterState. All validation occurs lock-free after copying.
 */
class AdapterResolver : public IAdapterResolver {
public:
    explicit AdapterResolver(ProtocolAdapterRegistry*          adapterRegistry,
                            AdapterLifecycleManager*          lifecycleManager = nullptr,
                            Binding::ProtocolBindingRegistry* bindingRegistry  = nullptr,
                            std::shared_ptr<Runtime::IExecutionClock> clock   = nullptr,
                            StorageEventBus*                  eventBus         = nullptr);

    ~AdapterResolver() override = default;

    void SetEventBus(StorageEventBus* eventBus);

    // ── IAdapterResolver ───────────────────────────────────────────────────

    AdapterResolutionResult Resolve(
        const Execution::ExecutionStep&        step,
        const Runtime::ExecutionRuntimeContext& context) override;

    AdapterResolutionResult Resolve(
        const Execution::ExecutionStep&        step,
        const Binding::ActionBinding&          binding,
        const Runtime::ExecutionRuntimeContext& context) override;

private:
    void PublishResolutionEvent(StorageEventType type,
                                const AdapterId& adapterId,
                                const std::string& stepId,
                                const std::string& detail = "");

    ProtocolAdapterRegistry*            m_adapterRegistry{nullptr};
    AdapterLifecycleManager*            m_lifecycleManager{nullptr};
    Binding::ProtocolBindingRegistry*   m_bindingRegistry{nullptr};
    std::shared_ptr<Runtime::IExecutionClock> m_clock;
    StorageEventBus*                    m_eventBus{nullptr};
};

} // namespace Protocol
} // namespace NetDiscovery
