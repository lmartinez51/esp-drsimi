/**
 * @file RuntimeExecutionEngine.cpp
 * @brief Implementation of RuntimeExecutionEngine implementing IExecutionRuntime (v5.0.0 Architecture Phase 9.2).
 */

#include "execution/RuntimeExecutionEngine.h"
#include "execution/ExecutionEvent.h"

#include <algorithm>

namespace NetDiscovery {
namespace Execution {

// ── Construction ─────────────────────────────────────────────────────────────

RuntimeExecutionEngine::RuntimeExecutionEngine(
        Runtime::RuntimeConfiguration               config,
        Runtime::ExecutionSessionRepository*        repository,
        std::shared_ptr<Runtime::IExecutionClock>   clock,
        std::shared_ptr<Runtime::IExecutionDispatcher> dispatcher,
        StorageEventBus*                            eventBus,
        Protocol::IAdapterResolver*                 adapterResolver)
    : m_config(std::move(config))
    , m_repository(repository)
    , m_clock(std::move(clock))
    , m_dispatcher(std::move(dispatcher))
    , m_eventBus(eventBus)
    , m_adapterResolver(adapterResolver) {

    if (!m_clock) {
        m_clock = std::make_shared<Runtime::SystemExecutionClock>();
    }
    if (!m_dispatcher) {
        m_dispatcher = std::make_shared<Runtime::NullExecutionDispatcher>();
    }
}


// ── Dependency Setters ────────────────────────────────────────────────────────

void RuntimeExecutionEngine::SetEventBus(StorageEventBus* eventBus) {
    m_eventBus = eventBus;
}

void RuntimeExecutionEngine::SetDispatcher(std::shared_ptr<Runtime::IExecutionDispatcher> dispatcher) {
    if (dispatcher) {
        m_dispatcher = std::move(dispatcher);
    }
}

void RuntimeExecutionEngine::SetClock(std::shared_ptr<Runtime::IExecutionClock> clock) {
    if (clock) {
        m_clock = std::move(clock);
    }
}

void RuntimeExecutionEngine::SetAdapterResolver(Protocol::IAdapterResolver* resolver) {
    m_adapterResolver = resolver;
}


// ── Repository Selector ───────────────────────────────────────────────────────

Runtime::ExecutionSessionRepository* RuntimeExecutionEngine::ActiveRepository() {
    return m_repository ? m_repository : &m_ownedRepository;
}

const Runtime::ExecutionSessionRepository* RuntimeExecutionEngine::ActiveRepository() const {
    return m_repository ? m_repository : &m_ownedRepository;
}

// ── Time Helper ───────────────────────────────────────────────────────────────

uint64_t RuntimeExecutionEngine::NowMs() const {
    return m_clock->NowMs();
}

// ── IExecutionRuntime — Session Lifecycle ─────────────────────────────────────

SessionId RuntimeExecutionEngine::StartSession(const ExecutionPlan& plan) {
    SessionId sessionId = "session." + plan.GetPlanId();

    ExecutionSession session(sessionId, plan.GetPlanId(), plan.GetRequestId(), NowMs());
    session.currentState = ExecutionPlanState::Ready;
    session.cursor = ExecutionCursor(&plan);

    for (const auto& step : plan.GetSteps()) {
        session.pendingSteps.push_back(step.GetStepId());
    }

    ActiveRepository()->Register(plan, std::move(session));

    if (m_config.diagnosticsEnabled) {
        m_diagnostics.RecordSessionCreated();
    }

    // Publish event using a snapshot read
    auto snap = ActiveRepository()->FindSession(sessionId);
    if (snap.has_value()) {
        PublishRuntimeEvent(StorageEventType::ExecutionStarted, snap.value());
    }

    return sessionId;
}

bool RuntimeExecutionEngine::StopSession(const SessionId& sessionId) {
    // Cancel if non-terminal before removal
    Cancel(sessionId);
    return ActiveRepository()->Remove(sessionId);
}

// ── IExecutionRuntime — State Machine Control ─────────────────────────────────

bool RuntimeExecutionEngine::Pause(const SessionId& sessionId) {
    bool result = false;

    ActiveRepository()->WithSession(sessionId, [&](ExecutionPlan& /*plan*/, ExecutionSession& session) {
        if (session.currentState == ExecutionPlanState::Running  ||
            session.currentState == ExecutionPlanState::Ready    ||
            session.currentState == ExecutionPlanState::Waiting) {
            session.currentState = ExecutionPlanState::Paused;
            PublishRuntimeEvent(StorageEventType::ExecutionPaused, session);
            result = true;
        }
    });

    return result;
}

bool RuntimeExecutionEngine::Resume(const SessionId& sessionId) {
    bool result = false;

    ActiveRepository()->WithSession(sessionId, [&](ExecutionPlan& /*plan*/, ExecutionSession& session) {
        if (session.currentState == ExecutionPlanState::Paused) {
            session.currentState = ExecutionPlanState::Ready;
            PublishRuntimeEvent(StorageEventType::ExecutionResumed, session);
            result = true;
        }
    });

    return result;
}

bool RuntimeExecutionEngine::Cancel(const SessionId& sessionId) {
    bool result = false;

    ActiveRepository()->WithSession(sessionId, [&](ExecutionPlan& /*plan*/, ExecutionSession& session) {
        if (!session.IsTerminal()) {
            session.currentState = ExecutionPlanState::Cancelled;
            session.runtimeContext.RequestCancellation();
            session.endTimestampMs = NowMs();
            PublishRuntimeEvent(StorageEventType::ExecutionCancelled, session);
            if (m_config.diagnosticsEnabled) {
                m_diagnostics.RecordSessionCancelled();
            }
            result = true;
        }
    });

    return result;
}

bool RuntimeExecutionEngine::EnqueueEvent(const SessionId& sessionId, Runtime::ExecutionEvent event) {
    bool result = false;

    ActiveRepository()->WithSession(sessionId, [&](ExecutionPlan& /*plan*/, ExecutionSession& session) {
        session.eventQueue.Push(std::move(event));
        result = true;
    });

    return result;
}

// ── IExecutionRuntime — Tick ───────────────────────────────────────────────────

ExecutionPlanState RuntimeExecutionEngine::Tick(const SessionId& sessionId) {
    uint64_t tickStart = NowMs();
    ExecutionPlanState resultState = ExecutionPlanState::Failed;

    ActiveRepository()->WithSession(sessionId, [&](ExecutionPlan& plan, ExecutionSession& session) {
        // 1. Process pending runtime events
        ProcessQueuedEvents(session);

        if (session.IsTerminal() || session.currentState == ExecutionPlanState::Paused) {
            resultState = session.currentState;
            return;
        }

        // 2. Evaluate ready steps from ExecutionScheduler
        std::vector<ExecutionStep> readySteps = m_scheduler.GetReadySteps(plan, session.cursor);

        if (readySteps.empty()) {
            if (!session.cursor.HasNext()) {
                session.currentState   = ExecutionPlanState::Completed;
                session.endTimestampMs = NowMs();
                PublishRuntimeEvent(StorageEventType::ExecutionCompleted, session);
                if (m_config.diagnosticsEnabled) {
                    m_diagnostics.RecordSessionCompleted();
                }
            } else {
                session.currentState = ExecutionPlanState::Waiting;
                PublishRuntimeEvent(StorageEventType::ExecutionWaiting, session);
            }
            resultState = session.currentState;
            return;
        }

        // 3. Dispatch ready steps
        session.currentState = ExecutionPlanState::Running;

        uint32_t dispatched = 0;
        for (const auto& step : readySteps) {
            if (dispatched >= m_config.maxConcurrentSteps) break;

            session.cursor.Advance(step.GetStepId());
            session.currentStepId = step.GetStepId();
            session.runtimeContext.SetStepStartTimestamp(step.GetStepId(), NowMs());

            PublishRuntimeEvent(StorageEventType::ExecutionStepStarted, session, step.GetStepId());

            if (m_config.diagnosticsEnabled) {
                m_diagnostics.RecordDispatcherCall();
            }

            // Phase 9.2 3-argument Dispatch: step, session, context
            Runtime::ExecutionStepResult dispResult =
                m_dispatcher->Dispatch(step, session, session.runtimeContext);

            if (dispResult.status == StepStatus::NotImplemented || dispResult.IsSuccess()) {
                ProcessCompletedStepInternal(plan, session, dispResult);
            } else {
                ProcessFailedStepInternal(plan, session, dispResult);
            }

            ++dispatched;
        }

        resultState = session.currentState;
    });

    if (m_config.diagnosticsEnabled) {
        uint32_t elapsed = static_cast<uint32_t>(m_clock->ElapsedMs(tickStart));
        m_diagnostics.RecordTickDuration(elapsed);
    }

    return resultState;
}

// ── Public Step Result Processing ─────────────────────────────────────────────

bool RuntimeExecutionEngine::ProcessCompletedStep(const SessionId& sessionId,
                                                   const Runtime::ExecutionStepResult& result) {
    bool ok = false;
    ActiveRepository()->WithSession(sessionId, [&](ExecutionPlan& plan, ExecutionSession& session) {
        ProcessCompletedStepInternal(plan, session, result);
        ok = true;
    });
    return ok;
}

bool RuntimeExecutionEngine::ProcessFailedStep(const SessionId& sessionId,
                                                const Runtime::ExecutionStepResult& result) {
    bool ok = false;
    ActiveRepository()->WithSession(sessionId, [&](ExecutionPlan& plan, ExecutionSession& session) {
        ProcessFailedStepInternal(plan, session, result);
        ok = true;
    });
    return ok;
}

// ── Private Step Helpers ──────────────────────────────────────────────────────

void RuntimeExecutionEngine::ProcessCompletedStepInternal(ExecutionPlan& plan,
                                                           ExecutionSession& session,
                                                           const Runtime::ExecutionStepResult& result) {
    session.cursor.MarkCompleted(result.stepId);
    session.completedSteps.push_back(result.stepId);
    session.runtimeContext.SetStepOutput(result.stepId, result.returnedValues);

    ExecutionStepResult legacyResult(result.status, result.stepId,
                                     result.executionDurationMs, result.errorMessage,
                                     result.returnedValues, result.metadata);
    session.stepResults[result.stepId] = legacyResult;

    auto pIt = std::find(session.pendingSteps.begin(), session.pendingSteps.end(), result.stepId);
    if (pIt != session.pendingSteps.end()) {
        session.pendingSteps.erase(pIt);
    }

    PublishRuntimeEvent(StorageEventType::ExecutionStepCompleted, session, result.stepId);

    if (!session.cursor.HasNext()) {
        session.currentState   = ExecutionPlanState::Completed;
        session.endTimestampMs = NowMs();
        PublishRuntimeEvent(StorageEventType::ExecutionCompleted, session);
        if (m_config.diagnosticsEnabled) {
            m_diagnostics.RecordSessionCompleted();
        }
    }
}

void RuntimeExecutionEngine::ProcessFailedStepInternal(ExecutionPlan& plan,
                                                        ExecutionSession& session,
                                                        const Runtime::ExecutionStepResult& result) {
    session.cursor.MarkFailed(result.stepId);
    session.failedSteps.push_back(result.stepId);

    ExecutionStepResult legacyResult(result.status, result.stepId,
                                     result.executionDurationMs, result.errorMessage,
                                     result.returnedValues, result.metadata);
    session.stepResults[result.stepId] = legacyResult;

    PublishRuntimeEvent(StorageEventType::ExecutionStepFailed, session, result.stepId);

    if (result.rollbackRequired ||
        (plan.GetExecutionPolicy().mode == ExecutionMode::RollbackOnFailure &&
         plan.GetCapabilities().supportsRollback)) {
        session.currentState = ExecutionPlanState::Rollback;
        PublishRuntimeEvent(StorageEventType::RollbackStarted, session, result.stepId);
        if (m_config.diagnosticsEnabled) {
            m_diagnostics.RecordRollback();
        }
    } else {
        session.currentState   = ExecutionPlanState::Failed;
        session.endTimestampMs = NowMs();
        PublishRuntimeEvent(StorageEventType::ExecutionFailed, session, result.stepId);
        if (m_config.diagnosticsEnabled) {
            m_diagnostics.RecordSessionFailed();
        }
    }
}

// ── IExecutionRuntime — Observation ───────────────────────────────────────────

std::optional<ExecutionSession> RuntimeExecutionEngine::GetSession(const SessionId& sessionId) const {
    return ActiveRepository()->FindSession(sessionId);
}

ExecutionPlanState RuntimeExecutionEngine::GetState(const SessionId& sessionId) const {
    auto snap = ActiveRepository()->FindSession(sessionId);
    if (!snap.has_value()) return ExecutionPlanState::Failed;
    return snap->currentState;
}

const Runtime::RuntimeDiagnostics& RuntimeExecutionEngine::GetDiagnostics() const {
    return m_diagnostics;
}

// ── Private Helpers ───────────────────────────────────────────────────────────

void RuntimeExecutionEngine::ProcessQueuedEvents(ExecutionSession& session) {
    while (!session.eventQueue.Empty()) {
        auto optEvt = session.eventQueue.Pop();
        if (!optEvt.has_value()) break;

        const auto& evt = optEvt.value();
        switch (evt.type) {
            case Runtime::ExecutionEventType::PauseRequested:
                if (!session.IsTerminal()) session.currentState = ExecutionPlanState::Paused;
                break;
            case Runtime::ExecutionEventType::ResumeRequested:
                if (session.currentState == ExecutionPlanState::Paused)
                    session.currentState = ExecutionPlanState::Ready;
                break;
            case Runtime::ExecutionEventType::CancelRequested:
                if (!session.IsTerminal()) {
                    session.currentState = ExecutionPlanState::Cancelled;
                    session.runtimeContext.RequestCancellation();
                    if (m_config.diagnosticsEnabled) {
                        m_diagnostics.RecordSessionCancelled();
                    }
                }
                break;
            case Runtime::ExecutionEventType::Timeout:
                if (!session.IsTerminal()) {
                    session.currentState = ExecutionPlanState::Timeout;
                    if (m_config.diagnosticsEnabled) {
                        m_diagnostics.RecordSessionTimeout();
                    }
                }
                break;
            default:
                break;
        }
    }
}

void RuntimeExecutionEngine::PublishRuntimeEvent(StorageEventType type,
                                                  const ExecutionSession& session,
                                                  const StepId& stepId) {
    if (!m_eventBus) return;

    StorageEvent event;
    event.type     = type;
    event.entityId = session.sessionId;
    event.timestamp = NowMs();
    event.metadata[RuntimeEventKeys::SessionId] = session.sessionId;
    event.metadata[RuntimeEventKeys::PlanId]    = session.planId;
    event.metadata[RuntimeEventKeys::RequestId] = session.requestId;
    event.metadata[RuntimeEventKeys::State]     = ToString(session.currentState);
    if (!stepId.empty()) {
        event.metadata[RuntimeEventKeys::StepId] = stepId;
    }

    m_eventBus->Publish(event);
}

} // namespace Execution
} // namespace NetDiscovery
