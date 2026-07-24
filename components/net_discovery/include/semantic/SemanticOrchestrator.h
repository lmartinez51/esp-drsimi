/**
 * @file SemanticOrchestrator.h
 * @brief Pure coordinator for the Phase E Intent Compiler pipeline.
 *
 * SemanticOrchestrator is a coordinator only. It delegates every responsibility
 * to owned collaborators and contains no plan-building, graph-traversal, or
 * step-creation logic.
 *
 * Pipeline:
 *   IntentDocument
 *     └─► IIntentCompiler        → IntentAST (ASTNode)
 *     └─► DeviceMatcher          → LogicalDevice
 *     └─► ControllerRegistry     → IDeviceController
 *     └─► PolicySelector         → ExecutionPolicy
 *     └─► IPlanBuilder           → IExecutionPlan (immutable)
 *     └─► IPlanOptimizer         → IExecutionPlan (optimized / pass-through)
 *     └─► ExecutionPlanExecutor  → ExecutionResult
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include "compiler/IntentDocument.h"
#include "compiler/IIntentCompiler.h"
#include "compiler/IPlanBuilder.h"
#include "compiler/IPlanOptimizer.h"
#include "semantic/SemanticError.h"
#include "semantic/DeviceMatcher.h"
#include "semantic/CapabilityFilter.h"
#include "plan/ExecutionPlanExecutor.h"
#include "plan/DefaultRuntimeDiagnostics.h"
#include "plan/IExecutionPlan.h"
#include "ControllerRegistry.h"
#include "ExecutionInfrastructure.h"
#include "core/ExecutionResult.h"
#include <memory>
#include <atomic>
#include <vector>

namespace semantic {

class SemanticOrchestrator {
public:
    /**
     * @brief Construct a fully-wired SemanticOrchestrator.
     * @param planExecutor         Frozen Phase C/D executor (required).
     * @param controllerRegistry   Controller lookup table (required).
     * @param executionInfra       Execution infrastructure (required).
     * @param intentCompiler       Phase E compiler: IntentDocument → ASTNode (required).
     * @param planBuilder          Phase E builder: ASTNode → IExecutionPlan (required).
     * @param planOptimizer        Optional optimizer (defaults to pass-through internally).
     */
    SemanticOrchestrator(
        std::shared_ptr<NetDiscovery::Plan::ExecutionPlanExecutor> planExecutor,
        std::shared_ptr<NetDiscovery::ControllerRegistry>          controllerRegistry,
        std::shared_ptr<NetDiscovery::ExecutionInfrastructure>     executionInfra,
        std::shared_ptr<NetDiscovery::compiler::IIntentCompiler>   intentCompiler,
        std::shared_ptr<NetDiscovery::compiler::IPlanBuilder>      planBuilder,
        std::shared_ptr<NetDiscovery::compiler::IPlanOptimizer>    planOptimizer = nullptr
    );

    /**
     * @brief Compile, build, optimize, and execute the intent described by @p document.
     *
     * Full pipeline:
     *   1. Compile IntentDocument → ASTNode via IIntentCompiler.
     *   2. Match device via DeviceMatcher.
     *   3. Select controller via ControllerRegistry.
     *   4. Select ExecutionPolicy via PolicySelector.
     *   5. Build IExecutionPlan via IPlanBuilder.
     *   6. Optimize via IPlanOptimizer.
     *   7. Execute via ExecutionPlanExecutor.
     *
     * @param document       Structured intent document from the LLM layer.
     * @param availableDevices All known logical devices (KnowledgeStore snapshot).
     * @param cancelToken    Optional cooperative cancellation token.
     * @return SemanticError::None on success.
     */
    SemanticError OrchestrateDocument(
        const NetDiscovery::compiler::IntentDocument&    document,
        const std::vector<NetDiscovery::LogicalDevice>&  availableDevices,
        std::shared_ptr<std::atomic<bool>>               cancelToken = nullptr
    );

    /**
     * @brief Execute a pre-compiled plan directly, bypassing the compiler/builder pipeline.
     * @param plan       Pre-compiled immutable plan.
     * @param cancelToken Optional cooperative cancellation token.
     * @return Execution result.
     */
    NetDiscovery::ExecutionResult ExecuteCompiledPlan(
        std::shared_ptr<NetDiscovery::Plan::IExecutionPlan> plan,
        std::shared_ptr<std::atomic<bool>>                  cancelToken = nullptr
    );

    // Legacy entry point — kept for backward compatibility with NetDiscoveryIPC.
    // Internally wraps the intent into an IntentDocument and calls OrchestrateDocument.
    SemanticError Orchestrate(
        const struct SemanticRequest&                    request,
        const std::vector<NetDiscovery::LogicalDevice>&  availableDevices,
        std::shared_ptr<std::atomic<bool>>               cancelToken = nullptr,
        const NetDiscovery::LogicalDevice*               targetDeviceOpt = nullptr
    );

private:
    std::shared_ptr<NetDiscovery::Plan::ExecutionPlanExecutor> m_planExecutor;
    std::shared_ptr<NetDiscovery::ControllerRegistry>          m_controllerRegistry;
    std::shared_ptr<NetDiscovery::ExecutionInfrastructure>     m_executionInfra;
    std::shared_ptr<NetDiscovery::compiler::IIntentCompiler>   m_intentCompiler;
    std::shared_ptr<NetDiscovery::compiler::IPlanBuilder>      m_planBuilder;
    std::shared_ptr<NetDiscovery::compiler::IPlanOptimizer>    m_planOptimizer;

    NetDiscovery::Plan::DefaultRuntimeDiagnostics              m_diagnostics;
    DeviceMatcher                                              m_deviceMatcher;
    CapabilityFilter                                           m_capabilityFilter;

    /// Select the best controller for a device from the registry.
    NetDiscovery::IDeviceController* SelectController(
        const NetDiscovery::LogicalDevice& device) const;
};

} // namespace semantic
