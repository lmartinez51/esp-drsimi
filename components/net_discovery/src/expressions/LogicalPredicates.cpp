/**
 * @file LogicalPredicates.cpp
 * @brief ValuePredicate and ComparePredicate implementations (v6.0 Phase D).
 */

#include "expressions/LogicalPredicates.h"
#include <variant>

using namespace NetDiscovery::Plan;

namespace NetDiscovery {
namespace Expressions {

// ---------------------------------------------------------------------------
// ValuePredicate
// ---------------------------------------------------------------------------
bool ValuePredicate::Evaluate(const ExecutionPlanContext& context,
                               const IVariableResolver&   resolver) const
{
    ExecutionValue val = m_expr->Evaluate(context, resolver);
    if (const bool*    b = std::get_if<bool>(&val))    return *b;
    if (const int64_t* i = std::get_if<int64_t>(&val)) return *i != 0;
    if (const double*  d = std::get_if<double>(&val))  return *d != 0.0;
    // monostate, string, or complex types → falsy
    return false;
}

// ---------------------------------------------------------------------------
// ComparePredicate
// ---------------------------------------------------------------------------
bool ComparePredicate::Evaluate(const ExecutionPlanContext& context,
                                 const IVariableResolver&   resolver) const
{
    // Compose a BinaryExpression on-the-fly and evaluate it
    BinaryExpression binExpr(m_op, m_left, m_right);
    ExecutionValue   result = binExpr.Evaluate(context, resolver);
    if (const bool* b = std::get_if<bool>(&result)) return *b;
    return false;
}

} // namespace Expressions
} // namespace NetDiscovery
