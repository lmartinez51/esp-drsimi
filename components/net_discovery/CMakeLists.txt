# ============================================================
# NetDiscovery ESP-IDF Component
# ============================================================

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
    "src/DeviceExecutor.cpp"
    "src/core/AuthenticationManager.cpp"
    "src/validation/ExecutionValidator.cpp"
    
    # ── Phase 5.5: Device Knowledge Store ─────────────
    "src/persistence/FileKnowledgeStore.cpp"
    "src/services/KnowledgeStore.cpp"
    "src/services/KnowledgeSynchronizer.cpp"

    # ── Phase 8: Parsing Framework ────────────────────
    "third_party/tinyxml2/tinyxml2.cpp"
    "src/parsing/soap/SOAPParserRegistry.cpp"
    "src/parsing/soap/SOAPResponseDispatcher.cpp"
    "src/parsing/soap/RenderingControlParser.cpp"
    "src/parsing/soap/AVTransportParser.cpp"
    "src/parsing/soap/ConnectionManagerParser.cpp"
    
    # ── Phase 10: Semantic Execution Layer ────────────
    "src/semantic/IntentCanonicalizer.cpp"
    "src/semantic/ParameterNormalizer.cpp"
    "src/semantic/LogicalEntityResolver.cpp"
    "src/semantic/DeviceMatcher.cpp"
    "src/semantic/CapabilityFilter.cpp"
    "src/semantic/WorkflowPlanner.cpp"
    "src/semantic/SemanticOrchestrator.cpp"
    "src/NetDiscoveryIPC.cpp"
)

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "include" "include/core" "src" "third_party/tinyxml2"
    REQUIRES lwip vfs esp_timer esp_system freertos esp_http_client pthread json esp_wifi
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
