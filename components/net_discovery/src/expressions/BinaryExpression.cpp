/**
 * @file BinaryExpression.cpp
 * @brief BinaryExpression::Evaluate implementation (v6.0 Phase D).
 */

#include "expressions/BinaryExpression.h"
#include "plan/ExecutionValue.h"
#include <variant>
#include <string>

using namespace NetDiscovery::Plan;

namespace NetDiscovery {
namespace Expressions {

// ---------------------------------------------------------------------------
// Internal helpers — no exceptions, no RTTI
// ---------------------------------------------------------------------------

static bool IsTruthy(const ExecutionValue& v) {
    if (const bool* b    = std::get_if<bool>(&v))    return *b;
    if (const int64_t* i = std::get_if<int64_t>(&v)) return *i != 0;
    if (const double* d  = std::get_if<double>(&v))  return *d != 0.0;
    return false;
}

static ExecutionValue ArithmeticOp(BinaryOperator op,
                                    const ExecutionValue& L,
                                    const ExecutionValue& R)
{
    // int64 + int64
    if (const int64_t* li = std::get_if<int64_t>(&L)) {
        if (const int64_t* ri = std::get_if<int64_t>(&R)) {
            switch (op) {
                case BinaryOperator::Add:      return ExecutionValue{*li + *ri};
                case BinaryOperator::Subtract: return ExecutionValue{*li - *ri};
                case BinaryOperator::Multiply: return ExecutionValue{*li * *ri};
                case BinaryOperator::Divide:
                    return (*ri != 0) ? ExecutionValue{*li / *ri} : ExecutionValue{};
                default: break;
            }
        }
        if (const double* rd = std::get_if<double>(&R)) {
            double ld = static_cast<double>(*li);
            switch (op) {
                case BinaryOperator::Add:      return ExecutionValue{ld + *rd};
                case BinaryOperator::Subtract: return ExecutionValue{ld - *rd};
                case BinaryOperator::Multiply: return ExecutionValue{ld * *rd};
                case BinaryOperator::Divide:
                    return (*rd != 0.0) ? ExecutionValue{ld / *rd} : ExecutionValue{};
                default: break;
            }
        }
    }
    // double + double (or double + int)
    if (const double* ld = std::get_if<double>(&L)) {
        double rd_val = 0.0;
        bool   has_r  = false;
        if (const double*  rd = std::get_if<double>(&R))  { rd_val = *rd;                     has_r = true; }
        if (const int64_t* ri = std::get_if<int64_t>(&R)) { rd_val = static_cast<double>(*ri); has_r = true; }
        if (has_r) {
            switch (op) {
                case BinaryOperator::Add:      return ExecutionValue{*ld + rd_val};
                case BinaryOperator::Subtract: return ExecutionValue{*ld - rd_val};
                case BinaryOperator::Multiply: return ExecutionValue{*ld * rd_val};
                case BinaryOperator::Divide:
                    return (rd_val != 0.0) ? ExecutionValue{*ld / rd_val} : ExecutionValue{};
                default: break;
            }
        }
    }
    return ExecutionValue{};
}

static ExecutionValue CompareOp(BinaryOperator op,
                                 const ExecutionValue& L,
                                 const ExecutionValue& R)
{
    // Equality works for all matching variant types
    if (op == BinaryOperator::Equal)    return ExecutionValue{L == R};
    if (op == BinaryOperator::NotEqual) return ExecutionValue{!(L == R)};

    // Ordered comparisons: numeric only
    auto toDouble = [](const ExecutionValue& v, double& out) -> bool {
        if (const int64_t* i = std::get_if<int64_t>(&v)) { out = static_cast<double>(*i); return true; }
        if (const double*  d = std::get_if<double>(&v))  { out = *d; return true; }
        return false;
    };

    double lv = 0.0, rv = 0.0;
    if (toDouble(L, lv) && toDouble(R, rv)) {
        switch (op) {
            case BinaryOperator::Less:           return ExecutionValue{lv <  rv};
            case BinaryOperator::LessOrEqual:    return ExecutionValue{lv <= rv};
            case BinaryOperator::Greater:        return ExecutionValue{lv >  rv};
            case BinaryOperator::GreaterOrEqual: return ExecutionValue{lv >= rv};
            default: break;
        }
    }
    // String comparison
    if (const std::string* ls = std::get_if<std::string>(&L)) {
        if (const std::string* rs = std::get_if<std::string>(&R)) {
            switch (op) {
                case BinaryOperator::Less:           return ExecutionValue{*ls <  *rs};
                case BinaryOperator::LessOrEqual:    return ExecutionValue{*ls <= *rs};
                case BinaryOperator::Greater:        return ExecutionValue{*ls >  *rs};
                case BinaryOperator::GreaterOrEqual: return ExecutionValue{*ls >= *rs};
                default: break;
            }
        }
    }
    return ExecutionValue{};
}

// ---------------------------------------------------------------------------
// BinaryExpression::Evaluate
// ---------------------------------------------------------------------------
ExecutionValue BinaryExpression::Evaluate(
    const ExecutionPlanContext& context,
    const IVariableResolver&   resolver) const
{
    // Short-circuit logical operators — do not evaluate right unless needed
    if (m_op == BinaryOperator::LogicalAnd) {
        ExecutionValue L = m_left->Evaluate(context, resolver);
        if (!IsTruthy(L)) return ExecutionValue{false};
        ExecutionValue R = m_right->Evaluate(context, resolver);
        return ExecutionValue{IsTruthy(R)};
    }
    if (m_op == BinaryOperator::LogicalOr) {
        ExecutionValue L = m_left->Evaluate(context, resolver);
        if (IsTruthy(L)) return ExecutionValue{true};
        ExecutionValue R = m_right->Evaluate(context, resolver);
        return ExecutionValue{IsTruthy(R)};
    }

    // All other operators: evaluate both children first
    ExecutionValue L = m_left->Evaluate(context, resolver);
    ExecutionValue R = m_right->Evaluate(context, resolver);

    switch (m_op) {
        case BinaryOperator::Add:
        case BinaryOperator::Subtract:
        case BinaryOperator::Multiply:
        case BinaryOperator::Divide:
            return ArithmeticOp(m_op, L, R);

        case BinaryOperator::Equal:
        case BinaryOperator::NotEqual:
        case BinaryOperator::Less:
        case BinaryOperator::LessOrEqual:
        case BinaryOperator::Greater:
        case BinaryOperator::GreaterOrEqual:
            return CompareOp(m_op, L, R);

        default:
            return ExecutionValue{};
    }
}

} // namespace Expressions
} // namespace NetDiscovery
