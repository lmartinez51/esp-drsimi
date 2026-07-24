/**
 * @file ExecutionPolicy.h
 * @brief Immutable execution policy value object and options (v5.1.0 Phase B).
 */

#pragma once

#include <chrono>
#include <cstdint>

namespace NetDiscovery {

enum class ReachabilityStrategy { None, VerifyOnce, WaitUntilReachable };
enum class RetryStrategy { None, Immediate, ExponentialBackoff, LinearBackoff };
enum class WakeStrategy { None, WakeOnLAN };
enum class AuthStrategy { None, Inherit, SessionToken };

struct ExecutionOptions {
    std::chrono::milliseconds reachabilityTimeoutMs{3000};
    std::chrono::milliseconds executionTimeoutMs{5000};
    int maxRetries{1};
    std::chrono::milliseconds retryBackoffMs{500};
};

class ExecutionPolicy {
public:
    ExecutionPolicy() = default;
    ExecutionPolicy(ReachabilityStrategy reach, RetryStrategy ret, WakeStrategy wk, AuthStrategy a, ExecutionOptions opt)
        : m_reachability(reach), m_retry(ret), m_wake(wk), m_auth(a), m_options(opt) {}

    ReachabilityStrategy GetReachabilityStrategy() const { return m_reachability; }
    RetryStrategy GetRetryStrategy() const { return m_retry; }
    WakeStrategy GetWakeStrategy() const { return m_wake; }
    AuthStrategy GetAuthStrategy() const { return m_auth; }
    const ExecutionOptions& GetOptions() const { return m_options; }

    static ExecutionPolicy Interactive() {
        ExecutionOptions opt;
        opt.reachabilityTimeoutMs = std::chrono::milliseconds(3000);
        opt.maxRetries = 2;
        return ExecutionPolicy(ReachabilityStrategy::VerifyOnce, RetryStrategy::ExponentialBackoff, WakeStrategy::None, AuthStrategy::Inherit, opt);
    }

    static ExecutionPolicy Background() {
        ExecutionOptions opt;
        opt.reachabilityTimeoutMs = std::chrono::milliseconds(10000);
        opt.maxRetries = 3;
        return ExecutionPolicy(ReachabilityStrategy::WaitUntilReachable, RetryStrategy::LinearBackoff, WakeStrategy::None, AuthStrategy::Inherit, opt);
    }

    static ExecutionPolicy FastFail() {
        ExecutionOptions opt;
        opt.maxRetries = 0;
        return ExecutionPolicy(ReachabilityStrategy::VerifyOnce, RetryStrategy::None, WakeStrategy::None, AuthStrategy::Inherit, opt);
    }

private:
    ReachabilityStrategy m_reachability{ReachabilityStrategy::None};
    RetryStrategy m_retry{RetryStrategy::None};
    WakeStrategy m_wake{WakeStrategy::None};
    AuthStrategy m_auth{AuthStrategy::Inherit};
    ExecutionOptions m_options;
};

} // namespace NetDiscovery
