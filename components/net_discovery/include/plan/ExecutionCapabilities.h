/**
 * @file ExecutionCapabilities.h
 * @brief Step capability trait reflection flags (v6.0 Phase C).
 */

#pragma once

namespace NetDiscovery {
namespace Plan {

struct ExecutionCapabilities {
    bool requiresNetwork{true};
    bool requiresDevice{true};
    bool supportsRollback{false};
    bool canRunParallel{true};
    bool producesOutput{false};
    bool mayBlock{false};

    static ExecutionCapabilities ActionStepDefaults() {
        ExecutionCapabilities caps;
        caps.requiresNetwork = true;
        caps.requiresDevice = true;
        caps.supportsRollback = false;
        caps.canRunParallel = true;
        return caps;
    }

    static ExecutionCapabilities ControlStepDefaults() {
        ExecutionCapabilities caps;
        caps.requiresNetwork = false;
        caps.requiresDevice = false;
        caps.supportsRollback = false;
        caps.canRunParallel = true;
        return caps;
    }
};

} // namespace Plan
} // namespace NetDiscovery
