/**
 * @file RuntimeTestHarness.h
 * @brief Complete Runtime Validation Harness for end-to-end verification (v5.0.0 Architecture Phase 16).
 */

#pragma once

#include "execution/RuntimeExecutionEngine.h"
#include "protocol/ProtocolAdapterDispatcher.h"
#include "protocol/AdapterResolver.h"
#include "protocol/ProtocolAdapterRegistry.h"
#include "protocol/connection/ConnectionPool.h"
#include "protocol/session/ProtocolSessionManager.h"
#include "transaction/TransactionManager.h"
#include "testing/FakeImplementations.h"
#include "testing/SyntheticPlanBuilder.h"
#include "testing/ExecutionReporter.h"

#include <memory>
#include <optional>

namespace NetDiscovery {
namespace Testing {

struct TestHarnessResult {
    bool passed{false};
    std::string scenarioName;
    Execution::SessionId sessionId;
    Execution::ExecutionPlanState finalState{Execution::ExecutionPlanState::Created};
    std::string executionReport;
};

/**
 * @brief Test harness assembling the entire runtime stack via dependency injection for end-to-end testing.
 */
class RuntimeTestHarness {
public:
    RuntimeTestHarness();

    void Initialize();

    TestHarnessResult RunSuccessfulExecutionScenario();
    TestHarnessResult RunCapabilityMismatchScenario();
    TestHarnessResult RunTransportTimeoutScenario();
    TestHarnessResult RunTransactionLifecycleScenario();

    const ExecutionReporter& GetReporter() const { return m_reporter; }

    Execution::SessionId ExecutePlanUntilCompletion(const Execution::ExecutionPlan& plan);

private:
    std::shared_ptr<Protocol::ConnectionPool>            m_connectionManager;
    std::shared_ptr<Protocol::ProtocolSessionManager>     m_sessionManager;
    std::shared_ptr<Transaction::TransactionManager>     m_transactionManager;
    std::shared_ptr<Protocol::ProtocolAdapterRegistry>   m_registry;
    std::shared_ptr<Protocol::AdapterResolver>           m_resolver;
    std::shared_ptr<Protocol::ProtocolAdapterDispatcher> m_dispatcher;
    std::unique_ptr<Execution::RuntimeExecutionEngine>   m_engine;

    ExecutionReporter m_reporter;
};

} // namespace Testing
} // namespace NetDiscovery
