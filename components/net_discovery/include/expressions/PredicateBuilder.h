/**
 * @file PredicateBuilder.h
 * @brief Fluent builder constructing IPredicate trees (v6.0 Phase D).
 */

#pragma once

#include "expressions/IPredicate.h"
#include "expressions/LogicalPredicates.h"
#include "expressions/ExpressionBuilder.h"

namespace NetDiscovery {
namespace Expressions {

class PredicateBuilder {
public:
    static std::shared_ptr<IPredicate> Value(std::shared_ptr<IExpression> expr) {
        return std::make_shared<ValuePredicate>(std::move(expr));
    }

    static std::shared_ptr<IPredicate> Variable(std::string key) {
        return std::make_shared<ValuePredicate>(ExpressionBuilder::Variable(std::move(key)));
    }

    static std::shared_ptr<IPredicate> Compare(std::shared_ptr<IExpression> left,
                                                BinaryOperator op,
                                                std::shared_ptr<IExpression> right) {
        return std::make_shared<ComparePredicate>(std::move(left), op, std::move(right));
    }

    static std::shared_ptr<IPredicate> And(std::shared_ptr<IPredicate> left, std::shared_ptr<IPredicate> right) {
        return std::make_shared<LogicalAndPredicate>(std::move(left), std::move(right));
    }

    static std::shared_ptr<IPredicate> Or(std::shared_ptr<IPredicate> left, std::shared_ptr<IPredicate> right) {
        return std::make_shared<LogicalOrPredicate>(std::move(left), std::move(right));
    }

    static std::shared_ptr<IPredicate> Not(std::shared_ptr<IPredicate> inner) {
        return std::make_shared<LogicalNotPredicate>(std::move(inner));
    }
};

} // namespace Expressions
} // namespace NetDiscovery
