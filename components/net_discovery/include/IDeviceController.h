/**
 * @file IDeviceController.h
 * @brief Abstract interface for metadata-only device controllers.
 */

#pragma once

#include "core/Capability.h"
#include "core/LogicalDevice.h"
#include "core/ResolutionDiagnostics.h"
#include "core/ActionDescriptor.h"
#include "core/ExecutionRoute.h"
#include <string>
#include <vector>
#include <optional>

namespace NetDiscovery {

/**
 * @brief Metadata-only controller interface for Phase 3.
 *
 * Exposes what a controller can do, and evaluates if it can
 * control a given device fingerprint.
 */
class IDeviceController {
public:
    virtual ~IDeviceController() = default;

    /**
     * @brief The name of the controller.
     */
    virtual std::string ControllerName() const = 0;

    /**
     * @brief Manufacturers this controller expects to support.
     */
    virtual std::vector<std::string> SupportedManufacturers() const = 0;

    /**
     * @brief Capabilities this controller can exercise.
     */
    virtual std::vector<Capability> SupportedCapabilities() const = 0;

    /**
     * @brief Stage 1: Mandatory Requirements Check.
     * @return True if the device explicitly meets all strict minimum requirements.
     */
    virtual bool IsMatch(const LogicalDevice& device) const = 0;

    /**
     * @brief Stage 2: Confidence Scoring.
     * @return Diagnostic object with breakdown. Returns 0 score if confidence cannot be established.
     */
    virtual ResolutionDiagnostics Evaluate(const LogicalDevice& device) const = 0;

    /**
     * @brief Stage 3: Endpoint Validation.
     * @return True if the controller can verify that the necessary physical endpoints exist.
     */
    virtual bool ValidateEndpoints(const LogicalDevice& device) const = 0;

    /**
     * @brief Vendor-specific actions that this controller uniquely adds beyond generic ones.
     */
    virtual std::vector<ActionDescriptor> VendorActions() const { return {}; }

    /**
     * @brief Provides the transport execution details for a specific action.
     */
    virtual std::optional<ExecutionRoute> GetExecutionRoute(
        const LogicalDevice& device, 
        const ActionDescriptor& action) const = 0;

};

} // namespace NetDiscovery
