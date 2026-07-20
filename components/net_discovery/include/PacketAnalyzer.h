/**
 * @file PacketAnalyzer.h
 * @brief Forwarding shim — canonical definition renamed to PacketUtilities.
 *
 * @deprecated Include "PacketUtilities.h" directly.
 *             The name "PacketAnalyzer" is reserved for IProtocolAnalyzer
 *             implementations. This file is kept for backward compatibility.
 */
#pragma once

#include "PacketUtilities.h"

namespace NetDiscovery {

// Type alias so existing code using PacketAnalyzer::FormatTimestamp() etc.
// continues to compile without modification.
using PacketAnalyzer = PacketUtilities;

} // namespace NetDiscovery
