#pragma once

namespace semantic {

/**
 * @brief Represents orchestration and semantic-level errors.
 */
enum class SemanticError
{
    None,
    DeviceNotFound,           // "I couldn't find a device matching 'The TV'"
    AmbiguousTarget,          // "I found 3 TVs, which one?"
    DeviceUnreachable,        // Target identified, but Pre-flight CheckReachable failed
    MissingCapability,        // Target identified, but lacks required features
    InvalidParameter,         // "Volume 900" is out of bounds
    WorkflowGenerationFailed, // Unknown CanonicalIntent or planner failure
    ExecutionFailed,          // Transport/Controller threw a hard error during execution
    Cancelled                 // User interrupted the workflow
};

} // namespace semantic
