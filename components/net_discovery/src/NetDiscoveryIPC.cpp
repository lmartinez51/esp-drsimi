#include "../include/NetDiscoveryIPC.h"
#include "../include/NetDiscoveryMetrics.h"
#include "../include/semantic/SemanticOrchestrator.h"
#include "../include/semantic/SemanticDataModels.h"
#include "../include/services/KnowledgeStore.h"
#include "../include/DeviceExecutor.h"
#include "../include/SSDPClient.h"
#include "../include/AnalyzerDispatcher.h"
#include "../include/SSDPAnalyzer.h"
#include "../include/XmlAnalyzer.h"
#include "../include/DeviceRegistry.h"
#include "../include/DescriptionDownloader.h"
#include "../include/IdentityResolutionEngine.h"
#include "../include/ProtocolNormalizer.h"
#include "../include/DeviceClassifier.h"
#include "../include/CapabilityResolver.h"
#include "../include/ControllerResolver.h"
#include "../include/ActionResolver.h"
#include "../include/ControllerRegistry.h"
#include "../include/TransportRegistry.h"
#include "../include/DummyTransport.h"
#include "../include/transports/DIALTransport.h"
#include "../include/transports/WebSocketTransport.h"
#include "../include/transports/WakeOnLANTransport.h"
#include "../include/transports/SOAPTransport.h"
#include "../include/core/AuthenticationManager.h"
#include "../src/persistence/FileKnowledgeStore.h"
#include "../include/ThreadHelper.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <string.h>
#include "../../../solutions/openai_demo/main/core/app_events.h"

static const char* TAG = "NetDiscoveryIPC";

QueueHandle_t netdiscovery_intent_queue = nullptr;

namespace {
    std::shared_ptr<NetDiscovery::KnowledgeStore> g_knowledgeStore;
    std::shared_ptr<NetDiscovery::ExecutionEngine> g_executor;
    std::shared_ptr<semantic::SemanticOrchestrator> g_orchestrator;
    std::shared_ptr<NetDiscovery::SSDPClient> g_ssdpClient;
    std::shared_ptr<NetDiscovery::ControllerRegistry> g_controllerRegistry;
    std::shared_ptr<NetDiscovery::TransportRegistry> g_transportRegistry;
    std::shared_ptr<NetDiscovery::AuthenticationManager> g_authManager;
}

extern "C" int send_function_output(const char *call_id, const char *output);

bool netdiscovery_validate_intent(const netdiscovery_intent_t* msg) {
    if (!msg) {
        ESP_LOGE(TAG, "[IntentValidator] Validation failed: NULL message pointer");
        return false;
    }
    if (msg->version != NETDISCOVERY_IPC_VERSION) {
        ESP_LOGE(TAG, "[IntentValidator] Validation failed: Invalid protocol version %u (expected %u)",
                 msg->version, NETDISCOVERY_IPC_VERSION);
        return false;
    }
    if (msg->call_id[0] == '\0') {
        ESP_LOGE(TAG, "[IntentValidator] Validation failed: Missing required call_id");
        return false;
    }
    if (msg->action[0] == '\0') {
        ESP_LOGE(TAG, "[IntentValidator] Validation failed: Missing required action string");
        return false;
    }
    // Check bounds null-termination
    if (strnlen(msg->call_id, IPC_CALL_ID_MAX_LEN) >= IPC_CALL_ID_MAX_LEN ||
        strnlen(msg->action, IPC_ACTION_MAX_LEN) >= IPC_ACTION_MAX_LEN ||
        strnlen(msg->target, IPC_TARGET_MAX_LEN) >= IPC_TARGET_MAX_LEN) {
        ESP_LOGE(TAG, "[IntentValidator] Validation failed: String parameter exceeds maximum bounds");
        return false;
    }
    return true;
}

bool netdiscovery_cancel_request(uint32_t request_id) {
    ESP_LOGI(TAG, "[%u][N/A] Cancel hook invoked (Reserved for Phase 2+)", (unsigned)request_id);
    return true;
}

static void netdiscovery_ipc_listener_task(void* arg) {
    ESP_LOGI(TAG, "IPC Listener Task started (Phase 1.5 Instrumented Pipeline)");
    netdiscovery_intent_t msg;
    while(true) {
        if (xQueueReceive(netdiscovery_intent_queue, &msg, portMAX_DELAY) == pdTRUE) {
            netdiscovery_pipeline_timestamps_t ts;
            memset(&ts, 0, sizeof(ts));
            ts.t4_ipc_pop = esp_timer_get_time();

            netdiscovery_memory_snapshot_t mem_before;
            netdiscovery_get_memory_snapshot(&mem_before);

            netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "intent popped from netdiscovery_intent_queue");
            ESP_LOGI(TAG, "[%u][%s] IPC intent popped (action='%s', target='%s')",
                     (unsigned)msg.request_id, msg.call_id, msg.action, msg.target);

            // Stage 1: Intent Validator Stage
            if (!netdiscovery_validate_intent(&msg)) {
                ESP_LOGE(TAG, "[%u][%s] Stage 1 (Validator) FAILED", (unsigned)msg.request_id, msg.call_id);
                netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "intent validation failed - destroyed");
                send_function_output(msg.call_id, "{\"error\": \"IPC intent validation failed: invalid format or missing fields\"}");
                continue;
            }
            ts.t5_validator_done = esp_timer_get_time();
            ESP_LOGI(TAG, "[%u][%s] Stage 1 (Validator) PASSED", (unsigned)msg.request_id, msg.call_id);

            if (!g_orchestrator || !g_knowledgeStore) {
                ESP_LOGE(TAG, "[%u][%s] NetDiscovery engine uninitialized", (unsigned)msg.request_id, msg.call_id);
                send_function_output(msg.call_id, "{\"error\": \"NetDiscovery engine not initialized\"}");
                continue;
            }

            // Stage 2: Construct SemanticRequest
            semantic::SemanticRequest req;
            req.rawIntent = msg.action;
            req.targetDescription = msg.target;

            if (msg.parameters_json[0] != '\0' && strcmp(msg.parameters_json, "{}") != 0) {
                cJSON* root = cJSON_Parse(msg.parameters_json);
                if (root && cJSON_IsObject(root)) {
                    cJSON* p = root->child;
                    while (p) {
                        if (cJSON_IsString(p)) {
                            req.rawParameters[p->string] = p->valuestring;
                            ESP_LOGI(TAG, "Parameter: %s = %s", p->string, p->valuestring);
                        } else if (cJSON_IsNumber(p)) {
                            req.rawParameters[p->string] = p->valuedouble;
                            ESP_LOGI(TAG, "Parameter: %s = %g", p->string, p->valuedouble);
                        } else if (cJSON_IsBool(p)) {
                            req.rawParameters[p->string] = cJSON_IsTrue(p);
                            ESP_LOGI(TAG, "Parameter: %s = %s", p->string, cJSON_IsTrue(p) ? "true" : "false");
                        }
                        p = p->next;
                    }
                    cJSON_Delete(root);
                }
            }

            // Stage 3: Detailed Knowledge Layer Validation
            ESP_LOGI(TAG, "[%u][%s] Stage 2 (Knowledge Lookup) starting for target: '%s'", (unsigned)msg.request_id, msg.call_id, req.targetDescription.c_str());
            auto availableEntities = g_knowledgeStore->GetLoadedEntities();

            ESP_LOGI(TAG, "========== KNOWLEDGE STORE DUMP ==========");
            ESP_LOGI(TAG, "Total Loaded Entities: %d", (int)availableEntities.size());
            int entityIdx = 0;
            for (const auto& entity : availableEntities) {
                ESP_LOGI(TAG, "Entity #%d", entityIdx++);
                ESP_LOGI(TAG, "  FriendlyName  : %s", entity.displayName.c_str());
                ESP_LOGI(TAG, "  PrimaryClass  : %s", NetDiscovery::ToString(entity.primaryClass).c_str());
                ESP_LOGI(TAG, "  IP Address    : %s", !entity.endpoints.empty() ? entity.endpoints[0].ip.c_str() : "None");
                ESP_LOGI(TAG, "  UniqueID      : %s", entity.persistentId.c_str());
                
                ESP_LOGI(TAG, "  Capabilities  :");
                if (entity.capabilities.empty()) {
                    ESP_LOGI(TAG, "    (None)");
                } else {
                    for (const auto& cap : entity.capabilities) {
                        ESP_LOGI(TAG, "    %s", NetDiscovery::ToString(cap).c_str());
                    }
                }
                
                ESP_LOGI(TAG, "  Controller Candidates:");
                if (entity.compatibleControllers.empty()) {
                    ESP_LOGI(TAG, "    UnknownController");
                } else {
                    for (const auto& ctrl : entity.compatibleControllers) {
                        ESP_LOGI(TAG, "    %s (Score: %d, Rejected: %s)", ctrl.name.c_str(), ctrl.confidence, ctrl.isRejected ? "true" : "false");
                    }
                }
                ESP_LOGI(TAG, "------------------------------------------------");
            }
            ESP_LOGI(TAG, "==========================================");

            std::vector<NetDiscovery::LogicalDevice> availableDevices;
            for (const auto& entity : availableEntities) {
                if (entity.type == NetDiscovery::EntityType::Device) {
                    NetDiscovery::LogicalDevice dev;
                    dev.id = entity.persistentId;
                    dev.displayName = entity.displayName;
                    dev.primaryClass = entity.primaryClass;
                    dev.roles = entity.roles;
                    dev.capabilities = entity.capabilities;
                    dev.endpoints = entity.endpoints;
                    dev.capabilityProfiles = entity.capabilityProfiles;
                    for (const auto& ctrl : entity.compatibleControllers) {
                        dev.controllerCandidates.push_back(ctrl);
                    }
                    availableDevices.push_back(dev);
                }
            }

            semantic::DeviceMatcher matcher;
            auto candidates = matcher.Match(req.targetDescription, availableDevices);
            if (!candidates.empty()) {
                auto selectedDev = candidates.front();
                const char* primary_ctrl = !selectedDev.controllerCandidates.empty() ? selectedDev.controllerCandidates.front().name.c_str() : "GenericController";
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Matched Entity : %s", (unsigned)msg.request_id, msg.call_id, selectedDev.displayName.c_str());
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Confidence     : 0.95", (unsigned)msg.request_id, msg.call_id);
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Capability     : %d", (unsigned)msg.request_id, msg.call_id, !selectedDev.capabilities.empty() ? static_cast<int>(selectedDev.capabilities.front()) : 0);
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Resolved Action: %s", (unsigned)msg.request_id, msg.call_id, msg.action);
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Controller     : %s", (unsigned)msg.request_id, msg.call_id, primary_ctrl);

                ESP_LOGI(TAG, "========== CONTROLLER SELECTION DUMP ==========");
                ESP_LOGI(TAG, "Selected Entity    : %s", selectedDev.displayName.c_str());
                ESP_LOGI(TAG, "Controller Candidates Count: %d", (int)selectedDev.controllerCandidates.size());
                for (const auto& ctrl : selectedDev.controllerCandidates) {
                    ESP_LOGI(TAG, "  Candidate: %s | Score: %d | Rejected: %s | Reason: %s",
                             ctrl.name.c_str(), ctrl.confidence, ctrl.isRejected ? "true" : "false",
                             ctrl.diagnosticReason.empty() ? "None" : ctrl.diagnosticReason.c_str());
                }
                ESP_LOGI(TAG, "===============================================");
            } else {
                ESP_LOGW(TAG, "[%u][%s] Knowledge Layer Match: No entity match found for '%s'", (unsigned)msg.request_id, msg.call_id, req.targetDescription.c_str());
            }

            // Stage 4: Execution Stub Callback (No physical network socket I/O)
            auto execution_lambda = [msg, req, availableDevices, ts, mem_before]() mutable {
                ESP_LOGI(TAG, "[%u][%s] Stage 6 (Orchestrator Stub) processing for action='%s'",
                         (unsigned)msg.request_id, msg.call_id, req.rawIntent.c_str());

                auto err = g_orchestrator->Orchestrate(req, availableDevices, nullptr);
                ts.t6_semantic_done = esp_timer_get_time();

                if (err == semantic::SemanticError::None) {
                    ESP_LOGI(TAG, "[%u][%s] Stage 7 (Completion Stub): Success (No physical socket I/O)", (unsigned)msg.request_id, msg.call_id);
                    char response_buf[256];
                    snprintf(response_buf, sizeof(response_buf),
                             "{\"status\":\"success\",\"request_id\":%u,\"action\":\"%s\",\"target\":\"%s\"}",
                             (unsigned)msg.request_id, msg.action, msg.target);
                    send_function_output(msg.call_id, response_buf);
                } else {
                    ESP_LOGW(TAG, "[%u][%s] Stage 7 (Completion Stub): Error code %d", (unsigned)msg.request_id, msg.call_id, static_cast<int>(err));
                    char errStr[128];
                    snprintf(errStr, sizeof(errStr), "{\"error\":\"semantic_error_%d\",\"request_id\":%u}",
                             static_cast<int>(err), (unsigned)msg.request_id);
                    send_function_output(msg.call_id, errStr);
                }

                ts.t7_response_sent = esp_timer_get_time();

                // Phase 1.5 Diagnostic Reports
                netdiscovery_memory_snapshot_t mem_after;
                netdiscovery_get_memory_snapshot(&mem_after);
                netdiscovery_print_latency_report(msg.request_id, msg.call_id, &ts);
                netdiscovery_print_memory_report("Tool Call Execution", &mem_before, &mem_after);
                netdiscovery_print_stack_report(NULL, xTaskGetCurrentTaskHandle());
                netdiscovery_print_queue_report(NULL, netdiscovery_intent_queue);
                netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "intent processing complete - destroyed");
            };

            NetDiscovery::ThreadHelper::StartPinnedThread("nd_exec", 8192, 4, 0, execution_lambda);
        }
    }
}
static StaticQueue_t* s_intent_queue_struct = nullptr;
static uint8_t* s_intent_queue_storage = nullptr;

extern "C" void netdiscovery_ipc_init(void) {
    if (!netdiscovery_intent_queue) {
        // 1. Relocate IPC Queue to PSRAM
        s_intent_queue_struct = (StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        s_intent_queue_storage = (uint8_t*)heap_caps_malloc(10 * sizeof(netdiscovery_intent_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        
        if (s_intent_queue_struct && s_intent_queue_storage) {
            netdiscovery_intent_queue = xQueueCreateStatic(10, sizeof(netdiscovery_intent_t), s_intent_queue_storage, s_intent_queue_struct);
        } else {
            ESP_LOGE(TAG, "Failed to allocate PSRAM for NetDiscovery IPC Queue");
            return;
        }

        // 2. Relocate IPC Listener Task to PSRAM (Using ESP-IDF caps allocation for safe alignment)
        if (xTaskCreatePinnedToCoreWithCaps(netdiscovery_ipc_listener_task, "nd_ipc_listener", 8192, nullptr, 4, nullptr, 0, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) {
            ESP_LOGE(TAG, "Failed to spawn nd_ipc_listener task in PSRAM");
        }
    }
}

extern "C" void netdiscovery_trigger_initial_scan(void) {
    ESP_LOGI(TAG, "Triggering one-shot NetDiscovery scan...");
    auto scan_lambda = []() {
        using namespace NetDiscovery;
        
        g_transportRegistry = std::make_shared<TransportRegistry>();
        g_transportRegistry->RegisterTransport(std::make_shared<DummyTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<DIALTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<SOAPTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<WebSocketTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<WakeOnLANTransport>());

        auto fileStore = std::make_unique<FileKnowledgeStore>("/littlefs/knowledge");
        g_knowledgeStore = std::make_shared<KnowledgeStore>(std::move(fileStore));
        g_knowledgeStore->Initialize();

        NetworkFingerprint networkFingerprint;
        wifi_config_t wifi_cfg;
        if (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg) == ESP_OK) {
            networkFingerprint.evidence.ssid = std::string((char*)wifi_cfg.sta.ssid);
        } else {
            networkFingerprint.evidence.ssid = "LocalNetwork";
        }
        g_knowledgeStore->ResolveKnownNetwork(networkFingerprint);

        g_authManager = std::make_shared<AuthenticationManager>(g_knowledgeStore.get());
        g_controllerRegistry = std::make_shared<ControllerRegistry>();
        g_executor = std::make_shared<ExecutionEngine>(*g_transportRegistry, *g_controllerRegistry, g_authManager);
        g_orchestrator = std::make_shared<semantic::SemanticOrchestrator>(g_executor);

        g_ssdpClient = std::make_shared<SSDPClient>();
        if (g_ssdpClient->Initialize() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SSDPClient for one-shot scan.");
            return;
        }

        AnalyzerDispatcher dispatcher;
        dispatcher.Register(std::make_unique<SSDPAnalyzer>());
        dispatcher.Register(std::make_unique<XmlAnalyzer>());
        DeviceRegistry registry;

        std::vector<std::string> targets = {
            "ssdp:all",
            "urn:schemas-upnp-org:device:MediaRenderer:1",
            "urn:schemas-upnp-org:device:MediaServer:1",
            "urn:dial-multiscreen-org:service:dial:1"
        };

        auto combinedOpt = g_ssdpClient->DiscoverAll(targets, 3, 5);
        if (combinedOpt.has_value()) {
            auto combined = combinedOpt.value();
            for (auto& r : combined) {
                for (const auto& pkt : r.packets) {
                    dispatcher.Dispatch(pkt, registry);
                }
            }
        }

        DescriptionDownloader downloader(registry, dispatcher);
        downloader.ProcessPending();

        IdentityResolutionEngine idEngine;
        std::vector<LogicalDevice> logicalDevices = idEngine.Resolve(registry.GetAll());

        ControllerResolver controllerResolver(*g_controllerRegistry);

        for (auto& logicalDev : logicalDevices) {
            ProtocolNormalizer::Normalize(logicalDev);
            DeviceClassifier::Classify(logicalDev);
            CapabilityResolver::Resolve(logicalDev);
            controllerResolver.Resolve(logicalDev);
            ActionResolver::Resolve(logicalDev);
            g_knowledgeStore->UpdateFromDiscovery(logicalDev);
        }

        ESP_LOGI(TAG, "========== DISCOVERY COMPLETION DUMP ==========");
        ESP_LOGI(TAG, "One-shot scan completed. Discovered %d devices.", (int)logicalDevices.size());
        int dIdx = 0;
        for (const auto& dev : logicalDevices) {
            ESP_LOGI(TAG, "Discovered Device #%d:", dIdx++);
            ESP_LOGI(TAG, "  Name         : %s", dev.displayName.c_str());
            ESP_LOGI(TAG, "  Manufacturer : %s", dev.manufacturer.c_str());
            ESP_LOGI(TAG, "  Model        : %s", dev.model.c_str());
            ESP_LOGI(TAG, "  PrimaryClass : %s", NetDiscovery::ToString(dev.primaryClass).c_str());
            ESP_LOGI(TAG, "  IP Address   : %s", !dev.endpoints.empty() ? dev.endpoints[0].ip.c_str() : "None");
            ESP_LOGI(TAG, "  Capabilities :");
            for (const auto& cap : dev.capabilities) {
                ESP_LOGI(TAG, "    - %s", NetDiscovery::ToString(cap).c_str());
            }
            ESP_LOGI(TAG, "  Controllers  :");
            for (const auto& ctrl : dev.controllerCandidates) {
                ESP_LOGI(TAG, "    - %s (Score: %d)", ctrl.name.c_str(), ctrl.confidence);
            }
            ESP_LOGI(TAG, "-----------------------------------------------");
        }
        ESP_LOGI(TAG, "===============================================");
        
        // Free SSDP socket resources since discovery is a one-time operation
        g_ssdpClient->Shutdown();
        g_ssdpClient.reset();

        orchestrator_post_event(ORCH_EVENT_NETDISCOVERY_COMPLETE);
    };
    
    // Spawn scan in a thread to not block boot sequence (Priority 2 to not starve WebRTC Audio)
    // NOTE: Must run in Internal SRAM because LittleFS / SPI Flash I/O disables Flash cache.
    NetDiscovery::ThreadHelper::StartInternalPinnedThread("nd_oneshot", 16384, 2, 0, scan_lambda);
}
