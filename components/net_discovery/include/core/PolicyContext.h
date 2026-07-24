/**
 * @file PolicyContext.h
 * @brief Context describing execution characteristics for PolicySelector (v5.1.0 Phase B).
 */

#pragma once

#include "ActionDescriptor.h"
#include "LogicalDevice.h"

namespace NetDiscovery {

enum class InteractionType { Interactive, Background, Streaming };
enum class RealtimeRequirement { LowLatency, Standard, Relaxed };
enum class ExecutionMode { Direct, Batched };

struct PolicyContext {
    InteractionType interaction{InteractionType::Interactive};
    RealtimeRequirement realtime{RealtimeRequirement::LowLatency};
    ExecutionMode mode{ExecutionMode::Direct};

    static PolicyContext FromAction(const ActionDescriptor& action) {
        PolicyContext ctx;
        if (action.id == ActionId::LaunchApplication || action.id == ActionId::SetVolume || action.id == ActionId::PowerOn) {
            ctx.interaction = InteractionType::Interactive;
            ctx.realtime = RealtimeRequirement::LowLatency;
        } else {
            ctx.interaction = InteractionType::Background;
            ctx.realtime = RealtimeRequirement::Standard;
        }
        return ctx;
    }
};

} // namespace NetDiscovery
