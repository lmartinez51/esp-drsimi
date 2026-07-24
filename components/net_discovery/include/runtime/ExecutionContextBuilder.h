/**
 * @file ExecutionContextBuilder.h
 * @brief Builder assembling ExecutionContext instances (v5.0.0 Architecture Hardening).
 */

#pragma once

#include "runtime/ExecutionContext.h"

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Builder responsible for assembling ExecutionContext aggregates.
 *
 * Contains ZERO state ownership and performs ZERO manager queries.
 */
class ExecutionContextBuilder {
public:
    ExecutionContextBuilder() = default;

    ExecutionContextBuilder& SetSession(const Execution::ExecutionSession* session) {
        m_context.session = session;
        return *this;
    }

    ExecutionContextBuilder& SetRuntimeContext(const ExecutionRuntimeContext* runtimeContext) {
        m_context.runtimeContext = runtimeContext;
        return *this;
    }

    ExecutionContextBuilder& SetTransaction(const Transaction::ExecutionTransaction* transaction) {
        m_context.transaction = transaction;
        return *this;
    }

    ExecutionContextBuilder& SetTransactionContext(const Transaction::ExecutionTransactionContext* transactionContext) {
        m_context.transactionContext = transactionContext;
        return *this;
    }

    ExecutionContextBuilder& SetProtocolSession(const Protocol::ProtocolSession* protocolSession) {
        m_context.protocolSession = protocolSession;
        return *this;
    }

    ExecutionContextBuilder& SetProtocolSessionContext(const Protocol::ProtocolSessionContext* protocolSessionContext) {
        m_context.protocolSessionContext = protocolSessionContext;
        return *this;
    }

    ExecutionContextBuilder& SetConnectionHandle(const Protocol::ConnectionHandle* connectionHandle) {
        m_context.connectionHandle = connectionHandle;
        return *this;
    }

    ExecutionContext Build() const {
        return m_context;
    }

private:
    ExecutionContext m_context;
};

} // namespace Runtime
} // namespace NetDiscovery
