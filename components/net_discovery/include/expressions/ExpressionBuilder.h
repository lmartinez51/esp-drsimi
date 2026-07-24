/**
 * @file ExpressionBuilder.h
 * @brief Fluent builder constructing IExpression trees (v6.0 Phase D).
 */

#pragma once

#include "expressions/IExpression.h"
#include "expressions/LiteralExpression.h"
#include "expressions/VariableExpression.h"
#include "expressions/UnaryExpression.h"
#include "expressions/BinaryExpression.h"
#include <memory>
#include <string>

namespace NetDiscovery {
namespace Expressions {

class ExpressionBuilder {
public:
    static std::shared_ptr<IExpression> Literal(NetDiscovery::Plan::ExecutionValue value) {
        return std::make_shared<LiteralExpression>(std::move(value));
    }

    static std::shared_ptr<IExpression> Variable(std::string key, ValueTypeTag typeHint = ValueTypeTag::Any) {
        return std::make_shared<VariableExpression>(VariableRef(std::move(key), typeHint));
    }

    static std::shared_ptr<IExpression> Unary(UnaryOperator op, std::shared_ptr<IExpression> child) {
        return std::make_shared<UnaryExpression>(op, std::move(child));
    }

    static std::shared_ptr<IExpression> Binary(BinaryOperator op,
                                                std::shared_ptr<IExpression> left,
                                                std::shared_ptr<IExpression> right) {
        return std::make_shared<BinaryExpression>(op, std::move(left), std::move(right));
    }
};

} // namespace Expressions
} // namespace NetDiscovery
