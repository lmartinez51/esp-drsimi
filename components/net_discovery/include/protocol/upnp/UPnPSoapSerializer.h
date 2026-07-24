/**
 * @file UPnPSoapSerializer.h
 * @brief Responsible exclusively for XML SOAP envelope generation, namespaces, and escaping (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include <string>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Pure XML SOAP serializer.
 *
 * Owns all XML generation logic, namespaces, XML entity escaping, and envelope creation.
 * Consumed by UPnPRequestBuilder. Contains ZERO HTTP formatting or networking logic.
 */
class UPnPSoapSerializer {
public:
    UPnPSoapSerializer() = default;

    /**
     * @brief Generates a complete XML SOAP 1.1 Envelope body string.
     */
    std::string SerializeEnvelope(const std::string& serviceType,
                                  const std::string& actionName,
                                  const std::unordered_map<std::string, std::string>& arguments) const;

    /**
     * @brief Escapes special XML characters (<, >, &, ", ').
     */
    std::string EscapeXml(const std::string& input) const;
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
