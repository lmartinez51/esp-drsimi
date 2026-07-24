/**
 * @file UPnPActionTranslator.cpp
 * @brief Implementation of UPnPActionTranslator (v5.0.0 Architecture Phase 11.1).
 */

#include "protocol/upnp/UPnPActionTranslator.h"

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

UPnPActionTranslation UPnPActionTranslator::Translate(const Execution::ExecutionStep& step) const {
    UPnPActionTranslation translation;
    translation.operationName = step.GetOperationId();
    translation.arguments     = step.GetParameterValues();
    return translation;
}

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
