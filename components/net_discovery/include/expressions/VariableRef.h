/**
 * @file VariableRef.h
 * @brief Lightweight typed reference to a blackboard key (v6.0 Phase D).
 *
 * VariableRef is a pure value type — a named pointer into ExecutionPlanContext
 * annotated with an expected type tag for validation and documentation.
 * It carries no behaviour; resolution is delegated to IVariableResolver.
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Expressions {

/// Type tag carried by VariableRef for optional validation during resolution.
enum class ValueTypeTag : uint8_t {
    Any = 0,
    Bool,
    Int64,
    Double,
    String,
    ExecutionResult,
    LogicalDevice
};

struct VariableRef {
    std::string  key;
    ValueTypeTag typeHint{ValueTypeTag::Any};

    explicit VariableRef(std::string k, ValueTypeTag t = ValueTypeTag::Any)
        : key(std::move(k)), typeHint(t) {}

    bool operator==(const VariableRef& o) const {
        return key == o.key && typeHint == o.typeHint;
    }
};

} // namespace Expressions
} // namespace NetDiscovery
