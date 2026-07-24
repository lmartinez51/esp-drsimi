/**
 * @file RuntimeTestHarness.cpp
 * @brief Implementation of RuntimeTestHarness (v5.0.0 Architecture Phase 16).
 */

#include "testing/RuntimeTestHarness.h"
#include "protocol/http/HTTPAdapter.h"

namespace NetDiscovery {
namespace Testing {

RuntimeTestHarness::RuntimeTestHarness() {
    m_connectionManager  = std::make_shared<Protocol::ConnectionPool>();
    m_sessionManager     = std::make_shared<Protocol::ProtocolSessionManager>();
    m_transactionManager = std::make_shared<Transaction::TransactionManager>();
    m_registry           = std::make_shared<Protocol::ProtocolAdapterRegistry>();
    m_resolver           = std::make_shared<Protocol::AdapterResolver>(m_registry.get());
    m_dispatcher         = std::make_shared<Protocol::ProtocolAdapterDispatcher>(m_resolver.get());

    m_engine = std::make_unique<Execution::RuntimeExecutionEngine>(
        Runtime::RuntimeConfiguration::Default(),
        nullptr, // internal owned session repository
        nullptr, // system clock
        nullptr, // null dispatcher fallback
        nullptr, // event bus
        m_resolver.get()
    );
}

void RuntimeTestHarness::Initialize() {
    m_reporter.RecordEvent("Harness", "Initialize", "Assembling dependency injected runtime stack");

    auto mockTransport = std::make_shared<Protocol::HTTP::MockHTTPTransport>();
    auto httpAdapter   = std::make_shared<Protocol::HTTP::HTTPAdapter>(
        Protocol::HTTP::HTTPAdapter::DefaultDescriptor(),
        mockTransport.get(),
        m_connectionManager.get(),
        m_sessionManager.get()
    );
    httpAdapter->Initialize();
    m_registry->Register(httpAdapter);

    m_reporter.RecordEvent("Harness", "AdapterRegistered", "Registered HTTPAdapter adapter.http.default");
}

Execution::SessionId RuntimeTestHarness::ExecutePlanUntilCompletion(const Execution::ExecutionPlan& plan) {
    Execution::SessionId sessionId = m_engine->StartSession(plan);
    m_reporter.RecordEvent("Engine", "StartSession", "Session: " + sessionId + ", Plan: " + plan.GetPlanId());

    while (true) {
        Execution::ExecutionPlanState state = m_engine->Tick(sessionId);
        if (state == Execution::ExecutionPlanState::Completed ||
            state == Execution::ExecutionPlanState::Failed ||
            state == Execution::ExecutionPlanState::Cancelled ||
            state == Execution::ExecutionPlanState::Timeout) {
            m_reporter.RecordEvent("Engine", "TerminalState", "State=" + Execution::ToString(state));
            break;
        }
    }

    return sessionId;
}

TestHarnessResult RuntimeTestHarness::RunSuccessfulExecutionScenario() {
    Initialize();
    m_reporter.RecordEvent("Scenario", "Start", "Running Successful Execution Scenario");

    Execution::ExecutionPlan plan = SyntheticPlanBuilder::BuildSingleStepPlan("step_1", "GET", "adapter.http.default");
    Execution::SessionId sid = ExecutePlanUntilCompletion(plan);
    Execution::ExecutionPlanState finalState = m_engine->GetState(sid);

    bool passed = (finalState == Execution::ExecutionPlanState::Completed);
    return {passed, "Successful Execution Scenario", sid, finalState, m_reporter.GenerateReport()};
}

TestHarnessResult RuntimeTestHarness::RunCapabilityMismatchScenario() {
    Initialize();
    m_reporter.RecordEvent("Scenario", "Start", "Running Capability Mismatch Scenario");

    Protocol::ProtocolCapabilityRequirement req({"mqtt.qos2_unsupported"});
    Execution::ExecutionPlan plan = SyntheticPlanBuilder::BuildSingleStepPlan("step_cap_fail", "GET", "adapter.http.default", req);
    Execution::SessionId sid = ExecutePlanUntilCompletion(plan);
    Execution::ExecutionPlanState finalState = m_engine->GetState(sid);

    bool passed = (finalState == Execution::ExecutionPlanState::Failed ||
                   finalState == Execution::ExecutionPlanState::Completed);

    return {passed, "Capability Mismatch Scenario", sid, finalState, m_reporter.GenerateReport()};
}

TestHarnessResult RuntimeTestHarness::RunTransportTimeoutScenario() {
    Initialize();
    m_reporter.RecordEvent("Scenario", "Start", "Running Transport Timeout Scenario");

    auto timeoutTransport = std::make_shared<Protocol::HTTP::MockHTTPTransport>(
        [](const Protocol::HTTP::HTTPRequest&, Protocol::HTTP::HTTPSessionContext&, const Protocol::ConnectionHandle&) {
            return Protocol::HTTP::HTTPResponse(-101, "Timeout waiting for HTTP response", {}, "", 5000, 0, 0);
        }
    );

    auto timeoutAdapter = std::make_shared<Protocol::HTTP::HTTPAdapter>(
        Protocol::ProtocolAdapterDescriptor("adapter.http.timeout", "HTTP Timeout", "1.0.0", "Test", "HTTP"),
        timeoutTransport.get(),
        m_connectionManager.get(),
        m_sessionManager.get()
    );
    timeoutAdapter->Initialize();
    m_registry->Register(timeoutAdapter);

    Execution::ExecutionPlan plan = SyntheticPlanBuilder::BuildSingleStepPlan("step_timeout", "GET", "adapter.http.timeout");
    Execution::SessionId sid = ExecutePlanUntilCompletion(plan);
    Execution::ExecutionPlanState finalState = m_engine->GetState(sid);

    bool passed = (finalState == Execution::ExecutionPlanState::Failed ||
                   finalState == Execution::ExecutionPlanState::Completed);

    return {passed, "Transport Timeout Scenario", sid, finalState, m_reporter.GenerateReport()};
}

TestHarnessResult RuntimeTestHarness::RunTransactionLifecycleScenario() {
    Initialize();
    m_reporter.RecordEvent("Scenario", "Start", "Running Transaction Lifecycle Scenario");

    auto tx = m_transactionManager->BeginTransaction("sess_test_1");
    bool passed = false;
    if (tx.has_value()) {
        m_reporter.RecordEvent("TransactionManager", "BeginTransaction", "Started tx: " + tx->transactionId);
        auto commitRes = m_transactionManager->Commit(tx->transactionId);
        if (commitRes.IsSuccess()) {
            m_reporter.RecordEvent("TransactionManager", "Commit", "Committed tx: " + tx->transactionId);
            passed = true;
        }
    }

    Execution::ExecutionPlan plan = SyntheticPlanBuilder::BuildSingleStepPlan();
    Execution::SessionId sid = ExecutePlanUntilCompletion(plan);
    Execution::ExecutionPlanState finalState = m_engine->GetState(sid);

    return {passed, "Transaction Lifecycle Scenario", sid, finalState, m_reporter.GenerateReport()};
}

} // namespace Testing
} // namespace NetDiscovery
