/**
 * @file RuntimeQualificationSuite.h
 * @brief Comprehensive Production Qualification Suite for ESP-Claw Execution Plan Runtime (v6.0 Phase D Qualification).
 *
 * Exercises all 14 architectural validation categories:
 *   1. Linear Workflow
 *   2. Conditional Workflow
 *   3. Loop Subsystem
 *   4. Parallel Execution
 *   5. Composite Workflows & Rollback
 *   6. EventWait Subsystem
 *   7. Expression Engine & Predicates
 *   8. Blackboard System
 *   9. Binding Service
 *  10. Failure Injection & Telemetry
 *  11. Stress & Node Scale (50, 100, 250, 500 nodes)
 *  12. Memory Ownership & Allocation Profile
 *  13. Determinism (1,000 consecutive executions)
 *  14. Architectural Invariants Verification
 */

#pragma once

#include "plan/ExecutionPlan.h"
#include "plan/ExecutionPlanInstance.h"
#include "plan/ExecutionScheduler.h"
#include "plan/ExecutionPlanExecutor.h"
#include "plan/ExecutionPlanVerifier.h"
#include "plan/ActionStep.h"
#include "plan/ControlSteps.h"
#include "plan/steps/WaitStep.h"
#include "plan/steps/EventWaitStep.h"
#include "plan/steps/BranchStep.h"
#include "plan/steps/SwitchStep.h"
#include "plan/steps/LoopStep.h"
#include "plan/steps/CompositeStep.h"
#include "plan/runners/StepRunnerRegistry.h"
#include "plan/binding/DefaultBindingResolver.h"
#include "plan/events/MockEventSignal.h"
#include "expressions/ExpressionBuilder.h"
#include "expressions/PredicateBuilder.h"
#include "expressions/DefaultExpressionEvaluator.h"
#include "expressions/BlackboardVariableResolver.h"

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace NetDiscovery {
namespace Testing {

struct SubsystemStatus {
    std::string subsystemName;
    std::string status; // "Production Ready", "Production Ready With Minor Recommendations", "Needs Correction"
    std::string findings;
};

struct QualificationCategoryResult {
    int categoryId{0};
    std::string categoryName;
    bool passed{false};
    int totalTests{0};
    int passedTests{0};
    int64_t executionTimeMs{0};
    std::string details;
};

struct StressBenchmarkMetrics {
    int nodeCount{0};
    int64_t buildTimeMs{0};
    int64_t executionTimeMs{0};
    size_t initialFreeHeap{0};
    size_t minFreeHeap{0};
    size_t peakMemoryBytes{0};
    size_t blackboardEntryCount{0};
};

struct QualificationSuiteReport {
    bool overallSuccess{false};
    int totalCategoriesTested{0};
    int categoriesPassed{0};
    int64_t totalSuiteDurationMs{0};
    std::vector<QualificationCategoryResult> categoryResults;
    std::vector<StressBenchmarkMetrics> stressMetrics;
    std::vector<SubsystemStatus> subsystemClassifications;
    std::string summaryText;
};

class RuntimeQualificationSuite {
public:
    static RuntimeQualificationSuite& Instance();

    QualificationSuiteReport RunAllQualifications();

    // Individual category validators
    QualificationCategoryResult ValidateLinearWorkflow();
    QualificationCategoryResult ValidateConditionalWorkflow();
    QualificationCategoryResult ValidateLoopWorkflow();
    QualificationCategoryResult ValidateParallelWorkflow();
    QualificationCategoryResult ValidateCompositeWorkflow();
    QualificationCategoryResult ValidateEventWaitWorkflow();
    QualificationCategoryResult ValidateExpressionEngine();
    QualificationCategoryResult ValidateBlackboard();
    QualificationCategoryResult ValidateBindingService();
    QualificationCategoryResult ValidateFailureHandling();
    QualificationCategoryResult ValidateStressWorkloads(std::vector<StressBenchmarkMetrics>& outStressMetrics);
    QualificationCategoryResult ValidateMemoryQualification();
    QualificationCategoryResult ValidateDeterminism();
    QualificationCategoryResult ValidateRuntimeInvariants();

private:
    RuntimeQualificationSuite() = default;
    
    std::shared_ptr<ExecutionInfrastructure> CreateMockInfrastructure();
    std::shared_ptr<Plan::IExecutionPlanObserver> CreateTelemetryCollector(std::vector<Plan::ExecutionEvent>& eventLog);
};

} // namespace Testing
} // namespace NetDiscovery
