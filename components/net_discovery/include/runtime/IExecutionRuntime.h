/**
 * @file IExecutionRuntime.h
 * @brief Frozen public interface for the Runtime Execution Engine (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

#include "execution/ExecutionPlan.h"
#include "execution/ExecutionSession.h"
#include "execution/ExecutionPlanState.h"
#include "runtime/ExecutionEvent.h"
#include "runtime/RuntimeDiagnostics.h"

#include <optional>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Frozen interface contract for the Runtime Execution Engine.
 *
 * All callers — orchestrators, AI layers, test harnesses — depend exclusively on
 * IExecutionRuntime. RuntimeExecutionEngine is an implementation detail.
 *
 * This interface is declared here and must not be modified after Phase 9.2.
 * Any future capability extension must be introduced as a new derived interface,
 * preserving backward compatibility.
 */
class IExecutionRuntime {
public:
    virtual ~IExecutionRuntime() = default;

    // ── Session Lifecycle ───────────────────────────────────────────────────

    /**
     * @brief Creates an active ExecutionSession from an immutable plan and registers it.
     * @return Unique SessionId for subsequent calls.
     */
    virtual Execution::SessionId StartSession(const Execution::ExecutionPlan& plan) = 0;

    /**
     * @brief Terminates and removes a session from the engine.
     *
     * If the session is non-terminal, it is cancelled before removal.
     * @return true if the session existed and was stopped.
     */
    virtual bool StopSession(const Execution::SessionId& sessionId) = 0;

    // ── State Machine Control ───────────────────────────────────────────────

    /**
     * @brief Advances the session state machine by one scheduling cycle.
     *
     * Processes queued events, evaluates ready steps, dispatches them, and
     * updates session state atomically within the call.
     * @return Current ExecutionPlanState after this Tick().
     */
    virtual Execution::ExecutionPlanState Tick(const Execution::SessionId& sessionId) = 0;

    /**
     * @brief Pauses an active session, preventing further step dispatch.
     * @return true if the session existed and was pauseable.
     */
    virtual bool Pause(const Execution::SessionId& sessionId) = 0;

    /**
     * @brief Resumes a paused session.
     * @return true if the session existed and was in Paused state.
     */
    virtual bool Resume(const Execution::SessionId& sessionId) = 0;

    /**
     * @brief Cancels an active session, marking it terminal immediately.
     * @return true if the session existed and was in a cancellable state.
     */
    virtual bool Cancel(const Execution::SessionId& sessionId) = 0;

    // ── Event Injection ─────────────────────────────────────────────────────

    /**
     * @brief Pushes a runtime event into the target session's event queue.
     *
     * Events are processed at the start of the next Tick() call.
     */
    virtual bool EnqueueEvent(const Execution::SessionId& sessionId, ExecutionEvent event) = 0;

    // ── Observation ─────────────────────────────────────────────────────────

    /**
     * @brief Returns an immutable snapshot of the session if registered.
     */
    virtual std::optional<Execution::ExecutionSession> GetSession(const Execution::SessionId& sessionId) const = 0;

    /**
     * @brief Returns the current state of a session without copying it.
     * @return ExecutionPlanState::Failed if the session does not exist.
     */
    virtual Execution::ExecutionPlanState GetState(const Execution::SessionId& sessionId) const = 0;

    /**
     * @brief Read-only access to accumulated runtime diagnostics.
     */
    virtual const RuntimeDiagnostics& GetDiagnostics() const = 0;
};

} // namespace Runtime
} // namespace NetDiscovery
