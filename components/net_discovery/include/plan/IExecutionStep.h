/**
 * @file IExecutionStep.h
 * @brief Base interface for all workflow step types with -fno-rtti support (v6.0 Phase C).
 */

#pragma once

#include "plan/ExecutionCapabilities.h"
#include "plan/ExecutionState.h"
#include "plan/binding/StepInputBinding.h"
#include "plan/binding/StepOutputDescriptor.h"
#include <string>
#include <vector>
#include <memory>

namespace NetDiscovery {
namespace Plan {

class IRollbackCapable; // Forward declaration

enum class StepType {
    Action,
    Condition,
    Delay,
    Wait,
    EventWait,
    Branch,
    Switch,
    Loop,
    Parallel,
    Composite,
    Custom
};

class IExecutionStep {
public:
    virtual ~IExecutionStep() = default;

    virtual std::string GetStepId() const = 0;
    virtual std::string GetStepName() const = 0;
    virtual StepType GetStepType() const = 0;
    virtual ExecutionCapabilities GetCapabilities() const = 0;

    virtual IRollbackCapable* AsRollbackCapable() { return nullptr; }

    virtual const std::vector<StepInputBinding>& GetInputBindings() const {
        static const std::vector<StepInputBinding> s_empty;
        return s_empty;
    }
    virtual const std::vector<StepOutputDescriptor>& GetOutputDescriptors() const {
        static const std::vector<StepOutputDescriptor> s_empty;
        return s_empty;
    }
    virtual const class StepMetadata& GetMetadata() const;
};

} // namespace Plan
} // namespace NetDiscovery
