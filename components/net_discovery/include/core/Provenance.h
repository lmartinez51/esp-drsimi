/**
 * @file Provenance.h
 * @brief Tracks the origin of metadata values.
 */

#pragma once

#include "DiscoverySource.h"

#include <string>
#include <map>

namespace NetDiscovery {

/**
 * @brief Maps a metadata field name (e.g. "manufacturer") to its source protocol.
 */
struct Provenance {
    std::map<std::string, DiscoverySource> sources;

    void Track(const std::string& field, DiscoverySource source) {
        sources[field] = source;
    }

    DiscoverySource GetSource(const std::string& field) const {
        auto it = sources.find(field);
        if (it != sources.end()) {
            return it->second;
        }
        return DiscoverySource::SSDP; // Default or Unknown
    }
};

} // namespace NetDiscovery
