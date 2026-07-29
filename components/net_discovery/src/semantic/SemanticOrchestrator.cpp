/**
 * @file SemanticOrchestrator.cpp
 * @brief Pure coordinator implementation for Phase E Intent Compiler pipeline.
 *
 * SemanticOrchestrator contains no plan-building, graph-traversal, or step-creation logic.
 * It sequences calls to IIntentCompiler, DeviceMatcher, ControllerRegistry,
 * PolicySelector, IPlanBuilder, IPlanOptimizer, and ExecutionPlanExecutor.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#include "semantic/SemanticOrchestrator.h"
#include "semantic/SemanticDataModels.h"
#include "compiler/DefaultIntentCompiler.h"
#include "compiler/DefaultPlanBuilder.h"
#include "compiler/PassThroughPlanOptimizer.h"
#include "compiler/IntentDocument.h"
#include "compiler/IPlanBuilder.h"
#include "plan/ExecutionPlanInstance.h"
#include "plan/CancellationToken.h"
#include "core/PolicyContext.h"
#include "core/PolicySelector.h"
#include "controllers/GenericDLNAController.h"
#include "controllers/SamsungController.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "SemanticOrchestrator";
extern "C" void netdiscovery_print_stack_report(const char* label, void* task);

// Static fallback controllers — used when the registry doesn't have a match.
static NetDiscovery::GenericDLNAController s_fallbackDLNAController;
static NetDiscovery::SamsungController     s_fallbackSamsungController;

namespace semantic {

// ============================================================================
// Constructor
// ============================================================================

SemanticOrchestrator::SemanticOrchestrator(
    std::shared_ptr<NetDiscovery::Plan::ExecutionPlanExecutor> planExecutor,
    std::shared_ptr<NetDiscovery::ControllerRegistry>          controllerRegistry,
    std::shared_ptr<NetDiscovery::ExecutionInfrastructure>     executionInfra,
    std::shared_ptr<NetDiscovery::compiler::IIntentCompiler>   intentCompiler,
    std::shared_ptr<NetDiscovery::compiler::IPlanBuilder>      planBuilder,
    std::shared_ptr<NetDiscovery::compiler::IPlanOptimizer>    planOptimizer)
    : m_planExecutor(std::move(planExecutor))
    , m_controllerRegistry(std::move(controllerRegistry))
    , m_executionInfra(std::move(executionInfra))
    , m_intentCompiler(intentCompiler
          ? std::move(intentCompiler)
          : std::make_shared<NetDiscovery::compiler::DefaultIntentCompiler>())
    , m_planBuilder(planBuilder
          ? std::move(planBuilder)
          : std::make_shared<NetDiscovery::compiler::DefaultPlanBuilder>())
    , m_planOptimizer(planOptimizer
          ? std::move(planOptimizer)
          : std::make_shared<NetDiscovery::compiler::PassThroughPlanOptimizer>())
{
    // Instantiate ExecutionPlanExecutor from infrastructure if not provided
    if (!m_planExecutor && m_executionInfra) {
        m_planExecutor = std::make_shared<NetDiscovery::Plan::ExecutionPlanExecutor>(
            m_executionInfra);
    }
    if (m_planExecutor) {
        m_planExecutor->RegisterObserver(
            std::make_shared<NetDiscovery::Plan::DefaultRuntimeDiagnostics>());
    }
}

// ============================================================================
// Primary pipeline entry point
// ============================================================================

SemanticError SemanticOrchestrator::OrchestrateDocument(
    const NetDiscovery::compiler::IntentDocument&    document,
    const std::vector<NetDiscovery::LogicalDevice>&  availableDevices,
    std::shared_ptr<std::atomic<bool>>               cancelToken)
{
    ESP_LOGI(TAG, "OrchestrateDocument: intentId='%s' name='%s'",
             document.intentId.c_str(), document.intentName.c_str());

    // ── Step 1: Compile IntentDocument → ASTNode tree ─────────────────────
    auto astRoot = m_intentCompiler->Compile(document);
    if (!astRoot) {
        ESP_LOGE(TAG, "Compile failed: null AST for intent '%s'", document.intentName.c_str());
        return SemanticError::WorkflowGenerationFailed;
    }

    // ── Step 2: Device resolution ──────────────────────────────────────────
    const std::string& deviceRef = document.root.targetDeviceRef;
    auto matches = m_deviceMatcher.Match(deviceRef, availableDevices);
    if (matches.empty()) {
        ESP_LOGE(TAG, "DeviceMatcher: no device found for ref='%s'", deviceRef.c_str());
        return SemanticError::DeviceNotFound;
    }

    // Resolve pointer into the original availableDevices vector (stable reference)
    const NetDiscovery::LogicalDevice* targetPtr = nullptr;
    for (const auto& dev : availableDevices) {
        if (dev.id == matches.front().id) {
            targetPtr = &dev;
            break;
        }
    }
    if (!targetPtr) {
        return SemanticError::DeviceNotFound;
    }

    // ── Step 3: Controller selection ───────────────────────────────────────
    NetDiscovery::IDeviceController* controller = SelectController(*targetPtr);
    if (!controller) {
        ESP_LOGE(TAG, "No controller found for device '%s'", targetPtr->displayName.c_str());
        return SemanticError::WorkflowGenerationFailed;
    }

    // ── Step 4: Policy selection ───────────────────────────────────────────
    NetDiscovery::ActionDescriptor primaryAction;
    primaryAction.id   = astRoot->resolvedAction;
    primaryAction.displayName = NetDiscovery::ToString(astRoot->resolvedAction);
    NetDiscovery::PolicyContext pctx = NetDiscovery::PolicyContext::FromAction(primaryAction);
    NetDiscovery::ExecutionPolicy policy = NetDiscovery::PolicySelector::SelectPolicy(pctx);

    // ── Step 5: Build IExecutionPlan ───────────────────────────────────────
    NetDiscovery::compiler::PlanBuildContext buildCtx;
    buildCtx.resolvedDevice    = targetPtr;
    buildCtx.selectedController = controller;
    buildCtx.policy            = policy;
    buildCtx.planId            = "plan-" + std::to_string(esp_timer_get_time());
    buildCtx.planName          = document.intentName;

    auto plan = m_planBuilder->Build(*astRoot, buildCtx);
    if (!plan) {
        ESP_LOGE(TAG, "PlanBuilder returned null plan");
        return SemanticError::WorkflowGenerationFailed;
    }

    // ── Step 6: Optimize ───────────────────────────────────────────────────
    auto optimized = m_planOptimizer->Optimize(plan);
    if (!optimized) {
        optimized = plan; // Fallback: use unoptimized plan
    }

    // ── Step 7: Execute ────────────────────────────────────────────────────
    auto result = ExecuteCompiledPlan(optimized, cancelToken);
    if (result.status != NetDiscovery::ExecutionStatus::Success) {
        ESP_LOGE(TAG, "ExecuteCompiledPlan failed: status=%d",
                 static_cast<int>(result.status));
        return SemanticError::ExecutionFailed;
    }

    ESP_LOGI(TAG, "OrchestrateDocument: SUCCESS intentId='%s'", document.intentId.c_str());
    return SemanticError::None;
}

// ============================================================================
// Execute a pre-compiled plan
// ============================================================================

NetDiscovery::ExecutionResult SemanticOrchestrator::ExecuteCompiledPlan(
    std::shared_ptr<NetDiscovery::Plan::IExecutionPlan> plan,
    std::shared_ptr<std::atomic<bool>>                  cancelToken)
{
    if (!plan) {
        ESP_LOGE(TAG, "ExecuteCompiledPlan: Plan is null");
        NetDiscovery::ExecutionResult r;
        r.status = NetDiscovery::ExecutionStatus::ExecutionFailed;
        return r;
    }

    std::string instanceId = "inst-" + plan->GetPlanId();

    NetDiscovery::Plan::ExecutionPlanInstance instance(
        instanceId, plan,
        NetDiscovery::Plan::CancellationToken(cancelToken));

    if (m_planExecutor) {
        return m_planExecutor->ExecutePlan(instance);
    }

    ESP_LOGE(TAG, "ExecuteCompiledPlan: m_planExecutor is null");
    NetDiscovery::ExecutionResult r;
    r.status = NetDiscovery::ExecutionStatus::ExecutionFailed;
    return r;
}

// ============================================================================
// Legacy backward-compatibility entry point
// ============================================================================

SemanticError SemanticOrchestrator::Orchestrate(
    const SemanticRequest&                           request,
    const std::vector<NetDiscovery::LogicalDevice>&  availableDevices,
    std::shared_ptr<std::atomic<bool>>               cancelToken,
    const NetDiscovery::LogicalDevice*               targetDeviceOpt)
{
    // Wrap SemanticRequest into an IntentDocument and delegate to OrchestrateDocument
    NetDiscovery::compiler::IntentDocument doc;
    doc.intentId   = "legacy-" + std::to_string(esp_timer_get_time());
    doc.intentName = request.rawIntent;
    doc.root.kind          = NetDiscovery::compiler::IntentNodeKind::SingleAction;
    doc.root.actionName    = request.rawIntent;
    doc.root.targetDeviceRef = request.targetDescription;
    doc.root.parameters    = request.rawParameters;

    return OrchestrateDocument(doc, availableDevices, cancelToken);
}

// ============================================================================
// Controller selection (private)
// ============================================================================

NetDiscovery::IDeviceController* SemanticOrchestrator::SelectController(
    const NetDiscovery::LogicalDevice& device) const
{
    // 1. Registry lookup via controller candidates
    if (m_controllerRegistry) {
        auto& controllers = m_controllerRegistry->GetControllers();
        for (const auto& candidate : device.controllerCandidates) {
            if (candidate.isRejected) continue;
            for (const auto& c : controllers) {
                if (c->ControllerName() == candidate.name) {
                    return c.get();
                }
            }
        }
    }

    // 2. Fallback: manufacturer heuristic
    std::string mfg = device.manufacturer;
    for (auto& ch : mfg) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (mfg.find("samsung") != std::string::npos) {
        return &s_fallbackSamsungController;
    }
    return &s_fallbackDLNAController;
}

} // namespace semantic
