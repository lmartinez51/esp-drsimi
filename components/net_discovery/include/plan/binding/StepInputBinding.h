/**
 * @file StepInputBinding.h
 * @brief Input binding declaration for IExecutionStep (v6.0 Phase D).
 */

#pragma once

#include "expressions/IExpression.h"
#include <string>
#include <memory>

namespace NetDiscovery {
namespace Plan {

struct StepInputBinding {
    std::string name;
    std::shared_ptr<NetDiscovery::Expressions::IExpression> expression;

    StepInputBinding(std::string n, std::shared_ptr<NetDiscovery::Expressions::IExpression> expr)
        : name(std::move(n)), expression(std::move(expr)) {}
};

} // namespace Plan
} // namespace NetDiscovery
