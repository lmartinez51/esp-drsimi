#pragma once

namespace NetDiscovery {

/**
 * @brief Standard service classifications.
 * 
 * Used to avoid scattering raw UPnP/DIAL string URIs throughout controllers
 * while ensuring stable, protocol-agnostic lookup capabilities.
 */
enum class StandardService {
    Unknown,

    RenderingControl,
    AVTransport,
    ConnectionManager,
    DIAL,
    RemoteControl
};

} // namespace NetDiscovery
