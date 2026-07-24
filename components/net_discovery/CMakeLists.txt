# ============================================================
# NetDiscovery ESP-IDF Component
# ============================================================

# Force Ninja to use response files to avoid Windows CreateProcess
# command-line length limits when the include path list is long.
set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")

set(srcs
    # ── Orchestrator ──────────────────────────────────
    "src/main.cpp"

    # ── Core layer helpers ────────────────────────────
    "src/Header.cpp"

    # ── Transport layer ───────────────────────────────
    "src/UdpSocket.cpp"
    "src/MulticastSocket.cpp"
    "src/SSDPClient.cpp"
    "src/TcpSocket.cpp"

    # ── Protocol analysis layer ───────────────────────
    "src/SSDPAnalyzer.cpp"
    "src/XmlAnalyzer.cpp"
    "src/AnalyzerDispatcher.cpp"

    # ── Utility layer ─────────────────────────────────
    "src/PacketUtilities.cpp"

    # ── Framework ──────────────
    "src/CaptureWriter.cpp"
    "src/HttpClient.cpp"
    "src/DescriptionDownloader.cpp"
    "src/DeviceRegistry.cpp"
    
    # ── Phase 3: Resolution Engine ────────────────────
    "src/ControllerRegistry.cpp"
    "src/ControllerResolver.cpp"

    # ── Phase 4.5: Architecture Consolidation ─────────
    "src/ProtocolNormalizer.cpp"
    "src/DeviceClassifier.cpp"
    "src/CapabilityResolver.cpp"
    "src/ActionResolver.cpp"

    # ── Phase 4: Device Fusion Engine ─────────────────
    "src/DeviceFusionEngine.cpp"
    "src/PresentationFormatter.cpp"
    "src/IdentityResolutionEngine.cpp"
    
    # ── Phase 5: Command Execution Framework ──────────
    "src/DummyTransport.cpp"
    "src/transports/DIALTransport.cpp"
    "src/transports/WebSocketTransport.cpp"
    "src/transports/websocket/SamsungWebSocketStrategy.cpp"
    "src/transports/WakeOnLANTransport.cpp"
    "src/transports/SOAPTransport.cpp"
    "src/transports/soap/builders/RenderingControlBuilder.cpp"
    "src/transports/soap/builders/AVTransportBuilder.cpp"
    "src/transports/soap/builders/ConnectionManagerBuilder.cpp"
    "src/TransportRegistry.cpp"
    "src/TransportSelector.cpp"
    "src/ExecutionEngine.cpp"
    "src/ExecutionInfrastructure.cpp"
    "src/core/PolicySelector.cpp"
    "src/infrastructure/DefaultReachabilityVerifier.cpp"
    "src/infrastructure/ReachabilityVerifierFactory.cpp"

    # ── Phase C & C.5: Execution Plan Runtime & Hardening ──
    "src/plan/DAGExecutionGraph.cpp"
    "src/plan/ExecutionPlanInstance.cpp"
    "src/plan/ExecutionScheduler.cpp"
    "src/plan/ExecutionPlanExecutor.cpp"
    "src/plan/ValidationReport.cpp"
    "src/plan/StaticPlanValidator.cpp"
    "src/plan/RuntimePlanValidator.cpp"
    "src/plan/ExecutionPlanVerifier.cpp"
    "src/plan/SchedulerValidator.cpp"
    "src/plan/DefaultRuntimeDiagnostics.cpp"

    # ── Phase D: Step Library, Expression Engine & Data Flow ──
    "src/expressions/UnaryExpression.cpp"
    "src/expressions/BinaryExpression.cpp"
    "src/expressions/LogicalPredicates.cpp"
    "src/plan/StepMetadata.cpp"
    "src/plan/StepRunnerRegistry.cpp"

    "src/DeviceExecutor.cpp"
    "src/core/AuthenticationManager.cpp"
    "src/validation/ExecutionValidator.cpp"
    
    # ── Phase 5, 6 & 7: StorageEventBus, KnowledgeGraph & CapabilitySet ──
    "src/core/StorageEventBus.cpp"
    "src/core/KnowledgeGraph.cpp"
    "src/core/CapabilitySet.cpp"
    
    # ── Phase 5.5 & Phase 2: In-Memory Multi-Index Persistence Core ──
    "src/persistence/FileKnowledgeStore.cpp"
    "src/persistence/AtomicFileWriter.cpp"
    "src/persistence/RepositoryContext.cpp"
    "src/persistence/JsonKnowledgeSerializer.cpp"
    "src/persistence/JsonAssetSerializer.cpp"
    "src/persistence/FileKnowledgeRepository.cpp"
    "src/persistence/InMemoryKnowledgeRepository.cpp"
    "src/persistence/NvsSettingsRepository.cpp"
    "src/persistence/FileAssetRepository.cpp"
    "src/persistence/StorageManager.cpp"
    "src/services/KnowledgeStore.cpp"
    "src/services/KnowledgeSynchronizer.cpp"
    "src/services/EntityResolutionEngine.cpp"
    "src/services/DeviceLifecycleManager.cpp"

    # ── Phase 8: Parsing Framework ────────────────────
    "third_party/tinyxml2/tinyxml2.cpp"
    "src/parsing/soap/SOAPParserRegistry.cpp"
    "src/parsing/soap/SOAPResponseDispatcher.cpp"
    "src/parsing/soap/RenderingControlParser.cpp"
    "src/parsing/soap/AVTransportParser.cpp"
    "src/parsing/soap/ConnectionManagerParser.cpp"
    
    # ── Phase E: Intent Compiler Pipeline ─────────────────────────────
    "src/compiler/DefaultIntentCompiler.cpp"
    "src/compiler/DefaultPlanBuilder.cpp"
    "src/compiler/PassThroughPlanOptimizer.cpp"

    # ── Phase 10: Semantic Execution Layer ────────────
    "src/semantic/IntentCanonicalizer.cpp"
    "src/semantic/IntentCanonicalizerTest.cpp"
    "src/semantic/ParameterNormalizer.cpp"
    "src/semantic/LogicalEntityResolver.cpp"
    "src/semantic/DeviceMatcher.cpp"
    "src/semantic/CapabilityFilter.cpp"
    "src/semantic/WorkflowPlanner.cpp"
    "src/semantic/SemanticOrchestrator.cpp"
    "src/NetDiscoveryIPC.cpp"
    "src/NetDiscoveryMetrics.cpp"

    # ── Phase 8 & 8.1: Protocol Action Binding Engine ─
    "src/binding/ProtocolBindingRegistry.cpp"
    "src/binding/BindingResolver.cpp"
    "src/binding/BindingSelector.cpp"

    # ── Phase 8.5 & 8.6: Execution Planning Engine ────
    "src/execution/ExecutionPlanner.cpp"
    "src/execution/PlanBuilder.cpp"
    "src/execution/PlanValidator.cpp"

    # ── Phase 9 & 9.1: Runtime Execution Engine ───────
    "src/execution/ExecutionScheduler.cpp"
    "src/execution/ExecutionDispatcher.cpp"
    "src/execution/RuntimeExecutionEngine.cpp"
    "src/runtime/NullExecutionDispatcher.cpp"

    # ── Phase 9.2: Runtime Finalization & Dispatcher Infrastructure ─
    "src/runtime/SystemExecutionClock.cpp"
    "src/runtime/ExecutionSessionRepository.cpp"

    # ── Phase 10 & 10.1: Protocol Adapter Framework & Resolver ─
    "src/protocol/ProtocolAdapterRegistry.cpp"
    "src/protocol/ProtocolAdapterFactory.cpp"
    "src/protocol/AdapterLifecycleManager.cpp"
    "src/protocol/ProtocolAdapterDispatcher.cpp"
    "src/protocol/AdapterResolver.cpp"

    # ── Phase 11 & 11.1: Concrete UPnP Protocol Adapter & Infrastructure ──
    "src/protocol/upnp/UPnPSoapSerializer.cpp"
    "src/protocol/upnp/UPnPActionTranslator.cpp"
    "src/protocol/upnp/UPnPRequestBuilder.cpp"
    "src/protocol/upnp/UPnPServiceResolver.cpp"
    "src/protocol/upnp/UPnPResponseParser.cpp"
    "src/protocol/upnp/UPnPErrorMapper.cpp"
    "src/protocol/upnp/UPnPRetryPolicy.cpp"
    "src/protocol/upnp/UPnPAdapter.cpp"

    # ── Phase 11.2: Connection Management Infrastructure ─────
    "src/protocol/connection/ConnectionPool.cpp"

    # ── Phase 12: MQTT Protocol Adapter Framework ────────────
    "src/protocol/mqtt/MQTTMessageBuilder.cpp"
    "src/protocol/mqtt/MQTTResponseMapper.cpp"
    "src/protocol/mqtt/MQTTRetryPolicy.cpp"
    "src/protocol/mqtt/MQTTAdapter.cpp"

    # ── Phase 12.1: Protocol Session Infrastructure ──────────
    "src/protocol/session/ProtocolSessionManager.cpp"

    # ── Phase 13: Protocol Capability Model & Validation ─────
    "src/protocol/capability/ProtocolCapabilityValidator.cpp"

    # ── Phase 14: Transaction Infrastructure & Atomic Execution ──
    "src/transaction/TransactionManager.cpp"

    # ── Phase 15: HTTP Protocol Adapter Framework ────────────
    "src/protocol/http/HTTPRequestBuilder.cpp"
    "src/protocol/http/HTTPResponseParser.cpp"
    "src/protocol/http/HTTPResponseMapper.cpp"
    "src/protocol/http/HTTPRetryPolicy.cpp"
    "src/protocol/http/HTTPAdapter.cpp"

    # ── Phase 16: Runtime Validation Harness & Hardware Demo ─
    "src/testing/ExecutionReporter.cpp"
    "src/testing/RuntimeTestHarness.cpp"
    "src/testing/RuntimeValidationDemo.cpp"
    "src/testing/RuntimeQualificationSuite.cpp"

    # ── Phase 17: Discovery Framework & Unified Device Registry ─
    "src/discovery/SSDPDiscoveryProvider.cpp"
    "src/discovery/ManualDiscoveryProvider.cpp"
    "src/discovery/DeviceRegistry.cpp"
    "src/discovery/DiscoveryManager.cpp"

    # ── Phase 18: Device Identity & Semantic Resolution ───────
    "src/identity/IdentityManager.cpp"

    # ── Phase 19: Validation Framework & Acceptance Infrastructure ─
    "src/validation/ValidationReporter.cpp"
    "src/validation/ValidationRunner.cpp"
    "src/validation/ScenarioDiscovery.cpp"
    "src/validation/ScenarioIdentity.cpp"
    "src/validation/ScenarioRuntimePipeline.cpp"
    "src/validation/ScenarioSamsungTV.cpp"
    "src/validation/ScenarioHTTP.cpp"
)


idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "include" "include/core" "include/binding" "include/execution" "include/runtime" "include/protocol" "include/protocol/upnp" "include/protocol/connection" "include/protocol/mqtt" "include/protocol/session" "include/protocol/capability" "include/transaction" "include/protocol/http" "include/testing" "include/discovery" "include/identity" "include/validation" "include/infrastructure" "include/plan" "include/expressions" "include/compiler" "src" "third_party/tinyxml2"
    REQUIRES lwip vfs esp_timer esp_system freertos esp_http_client pthread json esp_wifi nvs_flash
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
