/**
 * @file ExecutionContext.h
 * @brief Unified immutable view aggregating execution, transaction, session, and connection contexts (v5.0.0 Architecture Hardening).
 */

#pragma once

#include "execution/ExecutionSession.h"
#include "runtime/ExecutionRuntimeContext.h"
#include "transaction/ExecutionTransaction.h"
#include "transaction/ExecutionTransactionContext.h"
#include "protocol/session/ProtocolSession.h"
#include "protocol/session/ProtocolSessionContext.h"
#include "protocol/connection/ConnectionHandle.h"

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Immutable aggregate view grouping references to active execution structures.
 *
 * Contains ZERO ownership, ZERO lifecycle management, and ZERO execution logic.
 */
struct ExecutionContext {
    const Execution::ExecutionSession*               session{nullptr};
    const ExecutionRuntimeContext*                   runtimeContext{nullptr};
    const Transaction::ExecutionTransaction*         transaction{nullptr};
    const Transaction::ExecutionTransactionContext*  transactionContext{nullptr};
    const Protocol::ProtocolSession*                 protocolSession{nullptr};
    const Protocol::ProtocolSessionContext*          protocolSessionContext{nullptr};
    const Protocol::ConnectionHandle*                connectionHandle{nullptr};

    ExecutionContext() = default;

    ExecutionContext(const Execution::ExecutionSession*               sess,
                     const ExecutionRuntimeContext*                   rtCtx = nullptr,
                     const Transaction::ExecutionTransaction*         tx = nullptr,
                     const Transaction::ExecutionTransactionContext*  txCtx = nullptr,
                     const Protocol::ProtocolSession*                 pSess = nullptr,
                     const Protocol::ProtocolSessionContext*          pSessCtx = nullptr,
                     const Protocol::ConnectionHandle*                conn = nullptr)
        : session(sess)
        , runtimeContext(rtCtx)
        , transaction(tx)
        , transactionContext(txCtx)
        , protocolSession(pSess)
        , protocolSessionContext(pSessCtx)
        , connectionHandle(conn) {}

    bool HasSession() const { return session != nullptr; }
    bool HasTransaction() const { return transaction != nullptr; }
    bool HasProtocolSession() const { return protocolSession != nullptr; }
    bool HasConnection() const { return connectionHandle != nullptr && connectionHandle->IsValid(); }
};

} // namespace Runtime
} // namespace NetDiscovery
