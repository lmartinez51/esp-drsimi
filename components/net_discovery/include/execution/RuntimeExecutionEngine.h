/**
 * @file RuntimeExecutionEngine.h
 * @brief Deterministic state machine implementing IExecutionRuntime (v5.0.0 Architecture Phase 10.1).
 */

#pragma once

#include "runtime/IExecutionRuntime.h"
#include "runtime/IExecutionDispatcher.h"
#include "runtime/NullExecutionDispatcher.h"
#include "runtime/ExecutionSessionRepository.h"
#include "runtime/ExecutionClock.h"
#include "runtime/RuntimeConfiguration.h"
#include "runtime/RuntimeDiagnostics.h"
#include "execution/ExecutionScheduler.h"
#include "core/StorageEventBus.h"
#include "protocol/IAdapterResolver.h"

#include <memory>
#include <optional>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Concrete implementation of IExecutionRuntime.
 *
 * All dependencies are injected at construction. No global state. No singletons.
 * Session ownership is delegated to ExecutionSessionRepository.
 * Time is abstracted through IExecutionClock.
 * Adapter resolution is delegated to IAdapterResolver (no direct registry access).
 */
class RuntimeExecutionEngine : public Runtime::IExecutionRuntime {
public:
    /**
     * @brief Constructs the engine with all dependencies explicitly provided.
     *
     * @param config          Immutable runtime configuration.
     * @param repository      Session repository injected from outside; engine does not own it.
     * @param clock           Time source; defaults to SystemExecutionClock if nullptr.
     * @param dispatcher      Step dispatcher; defaults to NullExecutionDispatcher if nullptr.
     * @param eventBus        Optional event bus for lifecycle notifications.
     * @param adapterResolver Optional adapter resolver interface (Phase 10.1).
     */
    explicit RuntimeExecutionEngine(
        Runtime::RuntimeConfiguration               config            = Runtime::RuntimeConfiguration::Default(),
        Runtime::ExecutionSessionRepository*        repository        = nullptr,
        std::shared_ptr<Runtime::IExecutionClock>   clock             = nullptr,
        std::shared_ptr<Runtime::IExecutionDispatcher> dispatcher     = nullptr,
        StorageEventBus*                            eventBus          = nullptr,
        Protocol::IAdapterResolver*                 adapterResolver   = nullptr);

    ~RuntimeExecutionEngine() override = default;

    // ── Dependency Setters (post-construction injection) ────────────────────
    void SetEventBus(StorageEventBus* eventBus);
    void SetDispatcher(std::shared_ptr<Runtime::IExecutionDispatcher> dispatcher);
    void SetClock(std::shared_ptr<Runtime::IExecutionClock> clock);
    void SetAdapterResolver(Protocol::IAdapterResolver* resolver);

    // ── IExecutionRuntime ───────────────────────────────────────────────────
    SessionId StartSession(const ExecutionPlan& plan) override;
    bool StopSession(const SessionId& sessionId) override;

    ExecutionPlanState Tick(const SessionId& sessionId) override;

    bool Pause(const SessionId& sessionId) override;
    bool Resume(const SessionId& sessionId) override;
    bool Cancel(const SessionId& sessionId) override;

    bool EnqueueEvent(const SessionId& sessionId, Runtime::ExecutionEvent event) override;

    std::optional<ExecutionSession> GetSession(const SessionId& sessionId) const override;
    ExecutionPlanState GetState(const SessionId& sessionId) const override;

    const Runtime::RuntimeDiagnostics& GetDiagnostics() const override;

    // ── Step Result Processing ──────────────────────────────────────────────
    bool ProcessCompletedStep(const SessionId& sessionId, const Runtime::ExecutionStepResult& result);
    bool ProcessFailedStep(const SessionId& sessionId, const Runtime::ExecutionStepResult& result);

private:
    void ProcessQueuedEvents(ExecutionSession& session);
    void PublishRuntimeEvent(StorageEventType type, const ExecutionSession& session,
                             const StepId& stepId = "");
    void ProcessCompletedStepInternal(ExecutionPlan& plan, ExecutionSession& session,
                                      const Runtime::ExecutionStepResult& result);
    void ProcessFailedStepInternal(ExecutionPlan& plan, ExecutionSession& session,
                                   const Runtime::ExecutionStepResult& result);

    uint64_t NowMs() const;

    // ── Injected Dependencies ───────────────────────────────────────────────
    Runtime::RuntimeConfiguration                   m_config;
    Runtime::ExecutionSessionRepository*            m_repository{nullptr};
    std::shared_ptr<Runtime::IExecutionClock>       m_clock;
    std::shared_ptr<Runtime::IExecutionDispatcher>  m_dispatcher;
    StorageEventBus*                                m_eventBus{nullptr};
    // Phase 10.1: adapter resolver interface dependency only (zero registry/factory access)
    Protocol::IAdapterResolver*                     m_adapterResolver{nullptr};

    // ── Owned Components ────────────────────────────────────────────────────
    ExecutionScheduler                              m_scheduler;
    Runtime::RuntimeDiagnostics                    m_diagnostics;

    // ── Internal Session Storage (fallback when no repository is injected) ──
    Runtime::ExecutionSessionRepository             m_ownedRepository;
    Runtime::ExecutionSessionRepository*            ActiveRepository();
    const Runtime::ExecutionSessionRepository*     ActiveRepository() const;
};

} // namespace Execution
} // namespace NetDiscovery
