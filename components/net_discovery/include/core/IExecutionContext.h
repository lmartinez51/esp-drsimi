#pragma once

namespace NetDiscovery {

/**
 * @brief Base polymorphic context interface to carry protocol-specific 
 * execution data across the transport abstraction barrier.
 */
class IExecutionContext {
public:
    virtual ~IExecutionContext() = default;
};

} // namespace NetDiscovery
