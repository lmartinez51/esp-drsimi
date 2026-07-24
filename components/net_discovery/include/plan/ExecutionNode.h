/**
 * @file ExecutionNode.h
 * @brief Node wrapping an IExecutionStep within an ExecutionGraph (v6.0 Phase C).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include <memory>
#include <string>

namespace NetDiscovery {
namespace Plan {

class ExecutionNode {
public:
    explicit ExecutionNode(std::shared_ptr<IExecutionStep> step)
        : m_step(std::move(step)) {}

    const std::shared_ptr<IExecutionStep>& GetStep() const { return m_step; }
    std::string GetNodeId() const { return m_step ? m_step->GetStepId() : ""; }

private:
    std::shared_ptr<IExecutionStep> m_step;
};

} // namespace Plan
} // namespace NetDiscovery
