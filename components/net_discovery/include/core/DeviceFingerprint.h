/**
 * @file DeviceFingerprint.h
 * @brief Immutable snapshot of a device used for controller resolution.
 */

#pragma once

#include "Capability.h"
#include "DiscoverySource.h"

#include "Provenance.h"

#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Aggregated, immutable fingerprint of a device.
 * 
 * Used by the ControllerResolver to determine the best controller.
 */
class DeviceFingerprint {
public:
    DeviceFingerprint() = default;

    DeviceFingerprint(
        std::string manufacturer,
        std::string model,
        std::string friendlyName,
        std::string presentationUrl,
        std::string server,
        std::vector<std::string> deviceTypes,
        std::vector<std::string> serviceTypes,
        std::vector<std::string> namespaces,
        std::vector<std::string> normalizedFeatures,
        std::vector<Capability> capabilities,
        std::vector<DiscoverySource> discoverySources,
        Provenance provenance
    ) : m_manufacturer(std::move(manufacturer)),
        m_model(std::move(model)),
        m_friendlyName(std::move(friendlyName)),
        m_presentationUrl(std::move(presentationUrl)),
        m_server(std::move(server)),
        m_deviceTypes(std::move(deviceTypes)),
        m_serviceTypes(std::move(serviceTypes)),
        m_namespaces(std::move(namespaces)),
        m_normalizedFeatures(std::move(normalizedFeatures)),
        m_capabilities(std::move(capabilities)),
        m_discoverySources(std::move(discoverySources)),
        m_provenance(std::move(provenance)) {}

    const std::string& GetManufacturer() const { return m_manufacturer; }
    const std::string& GetModel() const { return m_model; }
    const std::string& GetFriendlyName() const { return m_friendlyName; }
    const std::string& GetPresentationUrl() const { return m_presentationUrl; }
    const std::string& GetServer() const { return m_server; }
    const std::vector<std::string>& GetDeviceTypes() const { return m_deviceTypes; }
    const std::vector<std::string>& GetServiceTypes() const { return m_serviceTypes; }
    const std::vector<std::string>& GetNamespaces() const { return m_namespaces; }
    const std::vector<std::string>& GetNormalizedFeatures() const { return m_normalizedFeatures; }
    const std::vector<Capability>& GetCapabilities() const { return m_capabilities; }
    const std::vector<DiscoverySource>& GetDiscoverySources() const { return m_discoverySources; }

    bool HasFeature(const std::string& feature) const {
        for (const auto& f : m_normalizedFeatures) {
            if (f == feature) return true;
        }
        return false;
    }

    bool HasCapability(Capability cap) const {
        for (const auto& c : m_capabilities) {
            if (c == cap) return true;
        }
        return false;
    }

    const Provenance& GetProvenance() const { return m_provenance; }

private:
    std::string m_manufacturer;
    std::string m_model;
    std::string m_friendlyName;
    std::string m_presentationUrl;
    std::string m_server;
    std::vector<std::string> m_deviceTypes;
    std::vector<std::string> m_serviceTypes;
    std::vector<std::string> m_namespaces;
    std::vector<std::string> m_normalizedFeatures;
    std::vector<Capability> m_capabilities;
    std::vector<DiscoverySource> m_discoverySources;
    Provenance m_provenance;
};

} // namespace NetDiscovery
