#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <variant>
#include <memory>
#include "core/LogicalDevice.h"
#include "core/ActionDescriptor.h"

namespace semantic {

// Parameter values in the Semantic Layer
using ExecutionParameterVariant = std::variant<int, double, bool, std::string>;

// Represents the parsed, raw intent from an AI/UI provider
struct SemanticRequest {
    std::string rawIntent;         
    std::string targetDescription; 
    std::map<std::string, std::string> rawParameters;
};

// Wait policy for workflow steps
enum class ExecutionWaitPolicy {
    None,
    FixedDelay,
    WaitUntilReachable,
    WaitForCapability,
    WaitForApplication,
    WaitForState
};

// Retry policy for workflow steps
struct RetryPolicy {
    int maxRetries;
    std::chrono::milliseconds backoff;
};

// A declarative step in an Execution Plan
struct ExecutionStep {
    std::string stepId;
    NetDiscovery::ActionDescriptor action;
    bool isOptional;               
    ExecutionWaitPolicy waitPolicy;
    std::chrono::milliseconds waitTimeout; 
    RetryPolicy retryPolicy;
};

// Represents a generated execution plan (purely declarative data)
struct ExecutionPlan {
    std::vector<NetDiscovery::LogicalDevice> targets;
    std::vector<ExecutionStep> steps;
};

} // namespace semantic
