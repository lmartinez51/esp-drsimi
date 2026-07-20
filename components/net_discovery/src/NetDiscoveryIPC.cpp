#include "../include/NetDiscoveryIPC.h"
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

#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include "../../../solutions/openai_demo/main/core/app_events.h"
#include <string.h>

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

static void netdiscovery_ipc_listener_task(void* arg) {
    ESP_LOGI(TAG, "IPC Listener Task started.");
    netdiscovery_intent_t msg;
    while(true) {
        if (xQueueReceive(netdiscovery_intent_queue, &msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received intent for call_id: %s", msg.call_id);
            if (!g_orchestrator || !g_knowledgeStore) {
                send_function_output(msg.call_id, "{\"error\": \"NetDiscovery engine not initialized\"}");
                continue;
            }

            cJSON* root = cJSON_Parse(msg.intent_json);
            if (!root) {
                send_function_output(msg.call_id, "{\"error\": \"Invalid JSON in intent\"}");
                continue;
            }
            
            semantic::SemanticRequest req;
            cJSON* action = cJSON_GetObjectItem(root, "action");
            if (cJSON_IsString(action)) req.rawIntent = action->valuestring;
            
            cJSON* target = cJSON_GetObjectItem(root, "target");
            if (cJSON_IsString(target)) req.targetDescription = target->valuestring;
            
            cJSON* params = cJSON_GetObjectItem(root, "parameters");
            if (params && cJSON_IsObject(params)) {
                cJSON* p = params->child;
                while (p) {
                    if (cJSON_IsString(p)) req.rawParameters[p->string] = p->valuestring;
                    else if (cJSON_IsNumber(p)) req.rawParameters[p->string] = p->valuedouble;
                    else if (cJSON_IsBool(p)) req.rawParameters[p->string] = cJSON_IsTrue(p);
                    p = p->next;
                }
            }
            cJSON_Delete(root);

            auto availableEntities = g_knowledgeStore->GetLoadedEntities();
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
            
            // Execute in detached thread (Core 0, slightly lower priority than audio)
            auto execution_lambda = [msg, req, availableDevices]() {
                auto err = g_orchestrator->Orchestrate(req, availableDevices, nullptr);
                
                if (err == semantic::SemanticError::None) {
                    send_function_output(msg.call_id, "{\"status\": \"success\"}");
                } else {
                    // Phase 5: Stale Device Fallback Sequence
                    if (err == semantic::SemanticError::DeviceUnreachable || err == semantic::SemanticError::ExecutionFailed) {
                        ESP_LOGW(TAG, "Device unreachable or execution failed. Initiating fallback sequence.");
                        bool recovered = false;
                        std::string targetUuid = "";
                        
                        // We need the logical device that was targeted to check WoL and UUID.
                        // For simplicity, we just do a quick match here.
                        semantic::DeviceMatcher matcher;
                        auto candidates = matcher.Match(req.targetDescription, availableDevices);
                        
                        if (!candidates.empty()) {
                            auto targetDev = candidates.front();
                            targetUuid = targetDev.id;
                            
                            // a. Attempt Targeted re-discovery
                            if (g_ssdpClient) {
                                ESP_LOGI(TAG, "Attempting targeted SSDP re-discovery for %s", targetUuid.c_str());
                                auto pktsOpt = g_ssdpClient->Discover("uuid:" + targetUuid, 2, 2);
                                if (pktsOpt.has_value() && !pktsOpt.value().empty()) {
                                    // Rediscovery found it! (In a full implementation we'd re-run pipeline on the packet and update KnowledgeStore)
                                    ESP_LOGI(TAG, "Targeted re-discovery succeeded! Re-attempting execution.");
                                    // Re-run Orchestrate
                                    auto retryErr = g_orchestrator->Orchestrate(req, availableDevices, nullptr);
                                    if (retryErr == semantic::SemanticError::None) {
                                        send_function_output(msg.call_id, "{\"status\": \"success\", \"note\": \"Recovered via targeted re-discovery\"}");
                                        recovered = true;
                                    }
                                }
                            }
                            
                            if (!recovered) {
                                // b. Check cached WakeOnLAN capability
                                bool supportsWoL = false;
                                for (const auto& cap : targetDev.capabilities) {
                                    if (cap == NetDiscovery::Capability::PowerControl) { // Or a specific WoL cap
                                        // We assume PowerControl implies WoL is a possibility for TVs
                                        supportsWoL = true; 
                                        break;
                                    }
                                }
                                
                                // c. Surface failure and WoL capability
                                if (supportsWoL) {
                                    send_function_output(msg.call_id, "{\"error\": \"Device is unreachable (likely powered off).\", \"wol_available\": true, \"prompt\": \"Would you like me to wake it up via Wake-on-LAN?\"}");
                                } else {
                                    send_function_output(msg.call_id, "{\"error\": \"Device is unreachable and does not support Wake-on-LAN.\"}");
                                }
                            }
                        } else {
                            send_function_output(msg.call_id, "{\"error\": \"Execution failed and device could not be identified for fallback.\"}");
                        }
                    } else {
                        // Semantic error (Missing capability, ambiguous target, etc)
                        std::string errStr = "{\"error\": \"semantic_error_" + std::to_string(static_cast<int>(err)) + "\"}";
                        send_function_output(msg.call_id, errStr.c_str());
                    }
                }
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

        ESP_LOGI(TAG, "One-shot scan completed. Discovered %d devices.", (int)logicalDevices.size());
        
        // Free SSDP socket resources since discovery is a one-time operation
        g_ssdpClient->Shutdown();
        g_ssdpClient.reset();

        orchestrator_post_event(ORCH_EVENT_NETDISCOVERY_COMPLETE);
    };
    
    // Spawn scan in a thread to not block boot sequence (Priority 2 to not starve WebRTC Audio)
    NetDiscovery::ThreadHelper::StartPinnedThread("nd_oneshot", 16384, 2, 0, scan_lambda);
}
