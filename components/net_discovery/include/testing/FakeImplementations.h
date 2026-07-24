/**
 * @file FakeImplementations.h
 * @brief Reusable fake/mock components for testing Runtime infrastructure (v5.0.0 Architecture Phase 16).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"
#include "protocol/connection/IConnectionManager.h"
#include "protocol/session/IProtocolSessionManager.h"
#include "transaction/ITransactionManager.h"
#include "protocol/http/IHTTPTransport.h"
#include "protocol/mqtt/IMQTTTransport.h"
#include "protocol/upnp/IUPnPTransport.h"

#include <vector>
#include <string>
#include <memory>
#include <atomic>

namespace NetDiscovery {
namespace Testing {

/**
 * @brief FakeProtocolAdapter implementing IProtocolAdapter for synthetic testing.
 */
class FakeProtocolAdapter : public Protocol::IProtocolAdapter {
public:
    explicit FakeProtocolAdapter(std::string adapterId = "adapter.fake",
                                 Protocol::ProtocolCapabilitySet capabilities = {})
        : m_descriptor(std::move(adapterId), "Fake", "1.0.0", "Testing", "Mock", {"TestOperation"}, {"testing"}, {})
        , m_capabilities(std::move(capabilities)) {}

    bool Initialize() override { m_initialized = true; return true; }
    void Shutdown() override { m_initialized = false; }
    bool IsAvailable() const override { return m_initialized; }

    const Protocol::ProtocolAdapterDescriptor& GetDescriptor() const override { return m_descriptor; }
    Runtime::DispatcherCapabilities GetCapabilities() const override { return Runtime::DispatcherCapabilities::BasicSynchronous(); }
    Protocol::ProtocolCapabilitySet GetProtocolCapabilities() const override { return m_capabilities; }

    Runtime::ExecutionStepResult Execute(
        const Execution::ExecutionStep&        step,
        Execution::ExecutionSession&           /*session*/,
        Runtime::ExecutionRuntimeContext&      /*context*/) override {

        m_invocationCount.fetch_add(1, std::memory_order_relaxed);
        m_lastExecutedStepId = step.GetStepId();

        if (m_failNextExecute) {
            return Runtime::ExecutionStepResult(step.GetStepId(), Execution::StepStatus::Failure, m_descriptor.adapterId, 10, 10, -1, "Fake execution failure");
        }

        return Runtime::ExecutionStepResult(step.GetStepId(), Execution::StepStatus::Success, m_descriptor.adapterId, 10, 10, 0, "");
    }

    uint32_t GetInvocationCount() const { return m_invocationCount.load(); }
    const std::string& GetLastExecutedStepId() const { return m_lastExecutedStepId; }
    void SetFailNextExecute(bool fail) { m_failNextExecute = fail; }

private:
    Protocol::ProtocolAdapterDescriptor m_descriptor;
    Protocol::ProtocolCapabilitySet     m_capabilities;
    bool m_initialized{true};
    bool m_failNextExecute{false};
    std::atomic<uint32_t> m_invocationCount{0};
    std::string m_lastExecutedStepId;
};

} // namespace Testing
} // namespace NetDiscovery
