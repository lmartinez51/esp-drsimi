/**
 * @file UnaryExpression.cpp
 * @brief UnaryExpression::Evaluate implementation (v6.0 Phase D).
 */

#include "expressions/UnaryExpression.h"
#include "plan/ExecutionValue.h"
#include <variant>

using namespace NetDiscovery::Plan;

namespace NetDiscovery {
namespace Expressions {

ExecutionValue UnaryExpression::Evaluate(
    const ExecutionPlanContext& context,
    const IVariableResolver&   resolver) const
{
    ExecutionValue childVal = m_child->Evaluate(context, resolver);

    switch (m_op) {
        case UnaryOperator::Negate:
            if (const int64_t* i = std::get_if<int64_t>(&childVal)) {
                return ExecutionValue{-(*i)};
            }
            if (const double* d = std::get_if<double>(&childVal)) {
                return ExecutionValue{-(*d)};
            }
            return ExecutionValue{};  // type mismatch → monostate

        case UnaryOperator::LogicalNot:
            if (const bool* b = std::get_if<bool>(&childVal)) {
                return ExecutionValue{!(*b)};
            }
            return ExecutionValue{};  // type mismatch → monostate
    }
    return ExecutionValue{};
}

} // namespace Expressions
} // namespace NetDiscovery
