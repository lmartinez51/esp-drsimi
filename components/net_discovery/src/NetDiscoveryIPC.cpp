#include "../include/NetDiscoveryIPC.h"
#include "../include/NetDiscoveryMetrics.h"
#include "../include/semantic/SemanticOrchestrator.h"
#include "../include/compiler/DefaultIntentCompiler.h"
#include "../include/compiler/DefaultPlanBuilder.h"
#include "../include/compiler/PassThroughPlanOptimizer.h"
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <algorithm>
#include <cctype>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
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

            // ── EARLY PIPELINE BRANCH: Administrative Intents (list_devices / forget_device) ──
            // Intercept BEFORE the KnowledgeStore dump and operational media pipeline.
            if (strcmp(msg.action, "list_devices") == 0 || strcmp(msg.action, "get_devices") == 0) {
                ESP_LOGI(TAG, "[%u][%s] \xf0\x9f\x9b\xa0\xef\xb8\x8f [AdminPipeline] Early branch for '%s'",
                         (unsigned)msg.request_id, msg.call_id, msg.action);
                char response_json[512];
                int count = g_knowledgeStore->FormatEntityListJson(response_json, sizeof(response_json));
                ESP_LOGI(TAG, "[AdminPipeline] Formatted %d entities into device list JSON", count);
                send_function_output(msg.call_id, response_json);
                continue; // BYPASS operational media pipeline completely
            }

            if (strcmp(msg.action, "forget_device") == 0 || strcmp(msg.action, "delete_device") == 0) {
                ESP_LOGI(TAG, "[%u][%s] \xf0\x9f\x9b\xa0\xef\xb8\x8f [AdminPipeline] Early branch for '%s' (target: '%s')",
                         (unsigned)msg.request_id, msg.call_id, msg.action, msg.target);
                ESP_LOGW(TAG, "[AdminPipeline] Internal Free: %u B, Largest Block: %u B",
                         (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                         (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

                // Build lowercase target entirely on the stack (no heap touch)
                char targetLower[IPC_TARGET_MAX_LEN];
                strlcpy(targetLower, msg.target, sizeof(targetLower));
                for (char* p = targetLower; *p; ++p)
                    *p = (char)std::tolower((unsigned char)*p);

                // Zero-copy in-place lookup inside KnowledgeStore — no vector copy, no heap allocation
                std::string matchedId;
                std::string matchedDisplay;
                int matchCount = g_knowledgeStore->FindEntityForAdmin(targetLower, matchedId, matchedDisplay);

                // Response: all on-stack buffers only
                char response_json[512];
                if (matchCount == 0) {
                    snprintf(response_json, sizeof(response_json),
                             "{\"status\":\"error\",\"code\":\"NOT_FOUND\",\"message\":\"Device '%s' not found\"}",
                             msg.target);
                    ESP_LOGW(TAG, "[AdminPipeline] Device '%s' not found", msg.target);
                    send_function_output(msg.call_id, response_json);
                } else if (matchCount > 1) {
                    snprintf(response_json, sizeof(response_json),
                             "{\"status\":\"error\",\"code\":\"AMBIGUOUS_TARGET\",\"message\":\"Multiple devices match '%s'. Please be more specific.\"}",
                             msg.target);
                    ESP_LOGW(TAG, "[AdminPipeline] Ambiguous target '%s' matched %d entities", msg.target, matchCount);
                    send_function_output(msg.call_id, response_json);
                } else {
                    // Cancel any pending LittleFS write before deleting (sanitized path matching GetEntityFilePath)
                    std::string safeId = matchedId;
                    for (char& c : safeId) {
                        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                            c = '_';
                        }
                    }
                    char cancelPath[128];
                    snprintf(cancelPath, sizeof(cancelPath),
                             "/littlefs/knowledge/%s/%s.json",
                             g_knowledgeStore->GetCurrentNetworkId().c_str(),
                             safeId.c_str());
                    netdiscovery_cancel_store_write(cancelPath);

                    bool removed = g_knowledgeStore->RemoveEntity(matchedId);
                    if (removed) {
                        snprintf(response_json, sizeof(response_json),
                                 "{\"status\":\"success\",\"message\":\"Device '%s' successfully removed\"}",
                                 matchedDisplay.c_str());
                        ESP_LOGI(TAG, "[AdminPipeline] Removed device '%s' (ID: %s)", matchedDisplay.c_str(), matchedId.c_str());
                    } else {
                        snprintf(response_json, sizeof(response_json),
                                 "{\"status\":\"error\",\"code\":\"REMOVE_FAILED\",\"message\":\"Failed to purge '%s'\"}",
                                 matchedDisplay.c_str());
                        ESP_LOGE(TAG, "[AdminPipeline] Failed to purge device '%s'", matchedDisplay.c_str());
                    }
                    send_function_output(msg.call_id, response_json);
                }

                continue; // BYPASS operational media pipeline completely
            }

            // Stage 3: Detailed Knowledge Layer Validation (operational intents only)
            ESP_LOGW(TAG, "[MEM CHECKPOINT A - Stage 2 Start] Internal Free: %u B, Largest Block: %u B",
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
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
                auto caps = entity.capabilities.GetCapabilities();
                if (caps.empty()) {
                    ESP_LOGI(TAG, "    (None)");
                } else {
                    for (const auto& cap : caps) {
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

            // ── OPERATIONAL MEDIA PIPELINE ─────────────────────────────────
            std::string lowerTarget = msg.target;
            for (auto& c : lowerTarget) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            bool isMediaTarget = (lowerTarget.find("tv") != std::string::npos ||
                                  lowerTarget.find("pantalla") != std::string::npos ||
                                  lowerTarget.find("television") != std::string::npos ||
                                  lowerTarget.find("media") != std::string::npos ||
                                  lowerTarget.find("youtube") != std::string::npos);

            std::vector<NetDiscovery::LogicalDevice> availableDevices;
            for (const auto& entity : availableEntities) {
                if (entity.type != NetDiscovery::EntityType::Device) continue;

                // Fast-Track Nivel 1: Descartar infraestructura de red no relevante para medios
                if (isMediaTarget) {
                    if (entity.primaryClass == NetDiscovery::PrimaryDeviceClass::InternetGateway ||
                        entity.primaryClass == NetDiscovery::PrimaryDeviceClass::Unknown) {
                        ESP_LOGI(TAG, "[Fast-Track L1] Discarded non-media entity: '%s' (%s)",
                                 entity.displayName.c_str(), NetDiscovery::ToString(entity.primaryClass).c_str());
                        continue;
                    }
                }

                // Fast-Track Nivel 1: Descartar entidades sin capacidades ni controladores válidos
                bool hasValidController = false;
                for (const auto& ctrl : entity.compatibleControllers) {
                    if (!ctrl.isRejected && ctrl.name != "UnknownController") {
                        hasValidController = true;
                        break;
                    }
                }
                if (entity.capabilities.Empty() && !hasValidController) {
                    ESP_LOGI(TAG, "[Fast-Track L1] Discarded capability-less entity: '%s'", entity.displayName.c_str());
                    continue;
                }

                NetDiscovery::LogicalDevice dev;
                dev.id = entity.persistentId;
                dev.displayName = entity.displayName;
                dev.manufacturer = entity.identity.vendor;
                dev.model = entity.identity.model;
                dev.serialNumber = entity.identity.serialNumber;
                dev.primaryClass = entity.primaryClass;
                dev.roles = entity.roles;
                dev.capabilities = entity.capabilities.GetCapabilities();
                dev.endpoints = entity.endpoints;
                dev.capabilityProfiles = entity.capabilityProfiles;
                dev.normalizedServices = entity.normalizedServices;
                dev.services = entity.services;
                
                for (const auto& ctrl : entity.compatibleControllers) {
                    dev.controllerCandidates.push_back(ctrl);
                }
                availableDevices.push_back(dev);
            }

            semantic::DeviceMatcher matcher;
            auto candidates = matcher.Match(req.targetDescription, availableDevices);
            ESP_LOGW(TAG, "[MEM CHECKPOINT B - Post Match] Internal Free: %u B, Largest Block: %u B",
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
            
            std::string matchedDevId = "";

            if (!candidates.empty()) {
                auto selectedDev = candidates.front();
                matchedDevId = selectedDev.id;

                // Fast-Track Nivel 2: Reducir availableDevices a las entidades emparejadas (1 o más), descartando el resto
                availableDevices = std::move(candidates);
                ESP_LOGI(TAG, "[Fast-Track L2] Target Pruned: availableDevices reduced to %d matched entities", (int)availableDevices.size());
                ESP_LOGW(TAG, "[MEM CHECKPOINT C - Post L2 Prune] Internal Free: %u B, Largest Block: %u B",
                         (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                         (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

                const char* primary_ctrl = !selectedDev.controllerCandidates.empty() ? selectedDev.controllerCandidates.front().name.c_str() : "GenericController";
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Matched Entity : %s", (unsigned)msg.request_id, msg.call_id, selectedDev.displayName.c_str());
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Confidence     : 0.95", (unsigned)msg.request_id, msg.call_id);
                ESP_LOGI(TAG, "[%u][%s] Knowledge Layer Capability     : %s", (unsigned)msg.request_id, msg.call_id, !selectedDev.capabilities.empty() ? selectedDev.capabilities.front().id.c_str() : "None");
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

            // Stage 4: Execution Callback (On-demand dynamic task using Bifurcated Orchestrator & Zero-Copy std::move)
            auto execution_lambda = [msg, req, availableDevices = std::move(availableDevices), matchedDevId, ts, mem_before]() mutable {
                ESP_LOGI(TAG, "[%u][%s] Stage 6 (Orchestrator) processing for action='%s'",
                         (unsigned)msg.request_id, msg.call_id, req.rawIntent.c_str());

                const NetDiscovery::LogicalDevice* safeDevPtr = nullptr;
                if (!matchedDevId.empty()) {
                    for (const auto& dev : availableDevices) {
                        if (dev.id == matchedDevId) {
                            safeDevPtr = &dev;
                            break;
                        }
                    }
                }

                auto cancelToken = std::make_shared<std::atomic<bool>>(false);
                auto err = g_orchestrator->Orchestrate(req, availableDevices, cancelToken, safeDevPtr);
                ts.t6_semantic_done = esp_timer_get_time();

                UBaseType_t highWaterMarkWords = uxTaskGetStackHighWaterMark(NULL);
                size_t unusedStackBytes = highWaterMarkWords * sizeof(StackType_t);
                size_t usedStackBytes = 8192 - unusedStackBytes;
                ESP_LOGW(TAG, "=========================================================");
                ESP_LOGW(TAG, "[STACK HIGH-WATER MARK] Task 'nd_exec': Allocated=8192 B, Used=%u B, Unused (Min Free)=%u B",
                         (unsigned)usedStackBytes, (unsigned)unusedStackBytes);
                ESP_LOGW(TAG, "=========================================================");

                if (err == semantic::SemanticError::None) {
                    ESP_LOGI(TAG, "[%u][%s] Stage 7 (Completion Stub): Success", (unsigned)msg.request_id, msg.call_id);
                    char response_buf[256];
                    snprintf(response_buf, sizeof(response_buf),
                             "{\"status\":\"success\",\"request_id\":%u,\"action\":\"%s\",\"target\":\"%s\"}",
                             (unsigned)msg.request_id, msg.action, msg.target);
                    send_function_output(msg.call_id, response_buf);
                } else {
                    ESP_LOGW(TAG, "[%u][%s] Stage 7 (Completion Stub): Error code %d", (unsigned)msg.request_id, msg.call_id, static_cast<int>(err));
                    char errStr[256];
                    const char* reasonStr = (err == semantic::SemanticError::DeviceNotFound) 
                        ? "Target device not found or offline" 
                        : (err == semantic::SemanticError::MissingCapability)
                        ? "Target device found, but does not support requested capability or feature"
                        : (err == semantic::SemanticError::ExecutionFailed)
                        ? "Target device was found on network, but network command connection was refused or failed"
                        : "Execution failed";
                    snprintf(errStr, sizeof(errStr), "{\"error\":\"semantic_error_%d\",\"reason\":\"%s\",\"request_id\":%u}",
                             static_cast<int>(err), reasonStr, (unsigned)msg.request_id);
                    send_function_output(msg.call_id, errStr);
                }

                ts.t7_response_sent = esp_timer_get_time();

                netdiscovery_memory_snapshot_t mem_after;
                netdiscovery_get_memory_snapshot(&mem_after);
                netdiscovery_print_latency_report(msg.request_id, msg.call_id, &ts);
                netdiscovery_print_memory_report("Tool Call Execution", &mem_before, &mem_after);
                netdiscovery_print_stack_report(NULL, xTaskGetCurrentTaskHandle());
                netdiscovery_print_queue_report(NULL, netdiscovery_intent_queue);
                netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "intent processing complete - destroyed");
            };

            ESP_LOGW(TAG, "[MEM CHECKPOINT D - Pre Thread Spawn] Internal Free: %u B, Largest Block: %u B",
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
            bool threadCreated = NetDiscovery::ThreadHelper::StartPinnedThread("nd_exec", 8192, 4, 0, execution_lambda);
            if (!threadCreated) {
                ESP_LOGE(TAG, "[%u][%s] FAILED to spawn dynamic 'nd_exec' task in PSRAM!",
                         (unsigned)msg.request_id, msg.call_id);
                char errStr[256];
                snprintf(errStr, sizeof(errStr),
                         "{\"error\":\"system_busy\",\"reason\":\"Failed to allocate execution thread under memory pressure\",\"request_id\":%u}",
                         (unsigned)msg.request_id);
                send_function_output(msg.call_id, errStr);
                netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "dynamic thread creation failed - error sent");
            }
        }
    }
}
// ============================================================================
// Static LittleFS store-writer task (Fix 2)
// SPI flash writes disable the cache, so file I/O must execute from a task
// whose stack lives in internal RAM. All persistence jobs are funneled through
// this statically-allocated task; producers hand over an owned heap buffer.
// ============================================================================

enum class StoreJobType {
    WRITE,
    DELETE
};

struct nd_write_job_t {
    StoreJobType type;
    char path[128];
    char* json_buf; // Buffer allocated by caller (e.g. PSRAM or Heap); freed by writer task
    size_t len;
};

static QueueHandle_t nd_write_queue = nullptr;
static StaticQueue_t nd_write_queue_struct;
static uint8_t nd_write_queue_storage[8 * sizeof(nd_write_job_t)];

// ESP-IDF FreeRTOS: StackType_t is uint8_t and stack depth is given in bytes
static StaticTask_t nd_store_writer_tcb;
static StackType_t nd_store_writer_stack[3072];

// Creates every missing parent directory of file_path. Runs exclusively in the
// writer-task context so stat/mkdir (flash I/O) execute from an internal-RAM stack.
static void nd_ensure_parent_dirs(const char* file_path) {
    char dir[sizeof(((nd_write_job_t*)0)->path)];
    strlcpy(dir, file_path, sizeof(dir));
    for (char* p = dir + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            struct stat st;
            if (stat(dir, &st) != 0) {
                if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
                    ESP_LOGE(TAG, "[StoreWriter] mkdir failed for %s (errno %d)", dir, errno);
                }
            }
            *p = '/';
        }
    }
}

static bool is_write_job_cancelled(const char* path);

static void nd_store_writer_task(void* arg) {
    nd_write_job_t job;
    for (;;) {
        if (xQueueReceive(nd_write_queue, &job, portMAX_DELAY) == pdTRUE) {
            if (job.type == StoreJobType::DELETE) {
                struct stat st;
                if (stat(job.path, &st) == 0) {
                    if (unlink(job.path) == 0) {
                        ESP_LOGI(TAG, "\xf0\x9f\x97\x91\xef\xb8\x8f [StoreWriter] Entity file deleted from LittleFS: %s", job.path);
                    } else {
                        ESP_LOGE(TAG, "[StoreWriter] Failed to delete %s (errno %d)", job.path, errno);
                    }
                }
                continue;
            }

            if (is_write_job_cancelled(job.path)) {
                ESP_LOGI(TAG, "🚫 [StoreWriter] Skipping cancelled write job for %s", job.path);
                free(job.json_buf);
                continue;
            }

            nd_ensure_parent_dirs(job.path);

            // ── SMART DIFF CHECK (Ejecutado de forma segura desde RAM Interna) ──
            struct stat st;
            if (stat(job.path, &st) == 0 && (size_t)st.st_size == job.len) {
                FILE* check_f = fopen(job.path, "rb");
                if (check_f) {
                    char* read_buf = (char*)heap_caps_malloc(job.len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                    if (!read_buf) read_buf = (char*)malloc(job.len);
                    if (read_buf) {
                        size_t r = fread(read_buf, 1, job.len, check_f);
                        fclose(check_f);
                        if (r == job.len && memcmp(read_buf, job.json_buf, job.len) == 0) {
                            free(read_buf);
                            free(job.json_buf);
                            ESP_LOGI(TAG, "⚡ [SmartCache] %s es 100%% idéntico. Escritura en Flash omitida.", job.path);
                            continue; // Omitir la escritura en Flash
                        }
                        free(read_buf);
                    } else {
                        fclose(check_f);
                    }
                }
            }
            // ───────────────────────────────────────────────────────────────────

            FILE* f = fopen(job.path, "wb");
            if (f) {
                size_t written = fwrite(job.json_buf, 1, job.len, f);
                fclose(f);
                if (written != job.len) {
                    ESP_LOGE(TAG, "[StoreWriter] Short write on %s (%u/%u bytes)",
                             job.path, (unsigned)written, (unsigned)job.len);
                } else {
                    ESP_LOGI(TAG, "💾 [StoreWriter] Entity JSON successfully persisted to LittleFS: %s (%u bytes)", 
                             job.path, (unsigned)written);
                }
            } else {
                ESP_LOGE(TAG, "[StoreWriter] fopen failed for %s", job.path);
            }
            free(job.json_buf);
        }
    }
}

extern "C" void netdiscovery_init_writer_task(void) {
    if (nd_write_queue) {
        return; // Already initialized
    }
    nd_write_queue = xQueueCreateStatic(8, sizeof(nd_write_job_t),
                                        nd_write_queue_storage, &nd_write_queue_struct);
    TaskHandle_t handle = xTaskCreateStaticPinnedToCore(
        nd_store_writer_task, "nd_store_writer", sizeof(nd_store_writer_stack),
        nullptr, 3, nd_store_writer_stack, &nd_store_writer_tcb, 0);
    if (!handle) {
        ESP_LOGE(TAG, "Failed to create nd_store_writer task");
    }
}

extern "C" bool netdiscovery_submit_store_write(const char* path, char* json_buf, size_t len) {
    if (!nd_write_queue || !path || !json_buf) {
        return false;
    }
    nd_write_job_t job = {};
    job.type = StoreJobType::WRITE;
    if (strlcpy(job.path, path, sizeof(job.path)) >= sizeof(job.path)) {
        ESP_LOGE(TAG, "[StoreWriter] Path too long: %s", path);
        return false;
    }
    job.json_buf = json_buf;
    job.len = len;
    if (xQueueSend(nd_write_queue, &job, 0) != pdTRUE) {
        ESP_LOGE(TAG, "[StoreWriter] Write queue full, dropping job for %s", path);
        return false; // Ownership stays with caller, who must free the buffer
    }
    return true; // Writer task now owns json_buf
}

extern "C" bool netdiscovery_submit_store_delete(const char* path) {
    if (!nd_write_queue || !path) {
        return false;
    }
    nd_write_job_t job = {};
    job.type = StoreJobType::DELETE;
    if (strlcpy(job.path, path, sizeof(job.path)) >= sizeof(job.path)) {
        ESP_LOGE(TAG, "[StoreWriter] Delete path too long: %s", path);
        return false;
    }
    if (xQueueSend(nd_write_queue, &job, 0) != pdTRUE) {
        ESP_LOGE(TAG, "[StoreWriter] Write queue full, dropping delete job for %s", path);
        return false;
    }
    return true;
}

#define MAX_CANCELLED_JOBS 8
static char s_cancelled_jobs[MAX_CANCELLED_JOBS][128];
static size_t s_cancelled_count = 0;
static portMUX_TYPE s_cancel_mux = portMUX_INITIALIZER_UNLOCKED;

static bool is_write_job_cancelled(const char* path) {
    if (!path || !path[0]) return false;
    bool cancelled = false;
    portENTER_CRITICAL(&s_cancel_mux);
    for (size_t i = 0; i < s_cancelled_count; i++) {
        if (strcmp(s_cancelled_jobs[i], path) == 0) {
            cancelled = true;
            for (size_t j = i; j < s_cancelled_count - 1; j++) {
                strlcpy(s_cancelled_jobs[j], s_cancelled_jobs[j + 1], 128);
            }
            s_cancelled_count--;
            break;
        }
    }
    portEXIT_CRITICAL(&s_cancel_mux);
    return cancelled;
}

extern "C" void netdiscovery_cancel_store_write(const char* path) {
    if (!path || !path[0]) return;
    portENTER_CRITICAL(&s_cancel_mux);
    if (s_cancelled_count < MAX_CANCELLED_JOBS) {
        strlcpy(s_cancelled_jobs[s_cancelled_count++], path, 128);
    }
    portEXIT_CRITICAL(&s_cancel_mux);
    ESP_LOGI(TAG, "🚫 [StoreWriter] Registered write cancel request for %s", path);
}

static StaticQueue_t* s_intent_queue_struct = nullptr;
static uint8_t* s_intent_queue_storage = nullptr;

extern "C" void netdiscovery_ipc_init(void) {
    // Fix 2: bring up the static LittleFS store-writer before any IPC consumers
    netdiscovery_init_writer_task();

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

extern "C" bool netdiscovery_trigger_initial_scan(void) {
    ESP_LOGI(TAG, "Triggering one-shot NetDiscovery scan...");

    // Fix 4: LittleFS *reads* (stat/opendir/fread) disable the flash cache just like
    // writes, so knowledge-store hydration must not run on the PSRAM-backed scan task.
    // It executes here, on the caller's internal-RAM stack, before the task is spawned.
    {
        using namespace NetDiscovery;

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
    }

    auto scan_lambda = []() {
        using namespace NetDiscovery;

        g_transportRegistry = std::make_shared<TransportRegistry>();
        g_transportRegistry->RegisterTransport(std::make_shared<DummyTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<DIALTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<SOAPTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<WebSocketTransport>());
        g_transportRegistry->RegisterTransport(std::make_shared<WakeOnLANTransport>());

        g_authManager = std::make_shared<AuthenticationManager>(g_knowledgeStore.get());
        g_controllerRegistry = std::make_shared<ControllerRegistry>();
        g_executor = std::make_shared<ExecutionEngine>(*g_transportRegistry, *g_controllerRegistry, g_authManager);
        // Phase E: construct compiler pipeline collaborators
        auto executionInfra   = std::make_shared<NetDiscovery::ExecutionInfrastructure>(g_executor);
        auto planExecutor     = std::make_shared<NetDiscovery::Plan::ExecutionPlanExecutor>(executionInfra);
        auto intentCompiler   = std::make_shared<NetDiscovery::compiler::DefaultIntentCompiler>();
        auto planBuilder      = std::make_shared<NetDiscovery::compiler::DefaultPlanBuilder>();
        auto planOptimizer    = std::make_shared<NetDiscovery::compiler::PassThroughPlanOptimizer>();

        g_orchestrator = std::make_shared<semantic::SemanticOrchestrator>(
            planExecutor,
            g_controllerRegistry,
            executionInfra,
            intentCompiler,
            planBuilder,
            planOptimizer
        );
        // ─────────────────────────────────────────────────────────────────────

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

        // Prune remaining unmerged skeletal/ghost candidates from logicalDevices
        auto isGhostDevice = [](const LogicalDevice& dev) {
            auto isBlank = [](const std::string& str) {
                return str.empty() || std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
            };
            bool emptyName = isBlank(dev.displayName) || dev.displayName == dev.id;
            bool emptyMfr = isBlank(dev.manufacturer);
            bool emptyModel = isBlank(dev.model);
            return emptyName && emptyMfr && emptyModel;
        };

        logicalDevices.erase(
            std::remove_if(logicalDevices.begin(), logicalDevices.end(), isGhostDevice),
            logicalDevices.end()
        );

        ControllerResolver controllerResolver(*g_controllerRegistry);

        size_t prev_internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t prev_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        ESP_LOGI(TAG, "🔍 [RAM Instrumentation] Before entity processing loop: INTERNAL free=%u B, largest=%u B",
                 (unsigned)prev_internal_free, (unsigned)prev_largest_block);

        int dev_count = 0;
        for (auto& logicalDev : logicalDevices) {
            ProtocolNormalizer::Normalize(logicalDev);
            DeviceClassifier::Classify(logicalDev);
            CapabilityResolver::Resolve(logicalDev);
            controllerResolver.Resolve(logicalDev);
            ActionResolver::Resolve(logicalDev);
            g_knowledgeStore->UpdateFromDiscovery(logicalDev);

            size_t curr_internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            size_t curr_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
            long free_delta = (long)prev_internal_free - (long)curr_internal_free;
            ESP_LOGI(TAG, "📊 [RAM Instrumentation] Entity #%d ('%s'): INTERNAL free=%u B (delta=%ld B), largest=%u B",
                     dev_count++, logicalDev.displayName.c_str(),
                     (unsigned)curr_internal_free, free_delta, (unsigned)curr_largest_block);
            prev_internal_free = curr_internal_free;
            prev_largest_block = curr_largest_block;
        }

        g_knowledgeStore->Consolidate();
        auto loadedEntities = g_knowledgeStore->GetLoadedEntities();

        ESP_LOGI(TAG, "========== DISCOVERY COMPLETION DUMP ==========");
        ESP_LOGI(TAG, "One-shot scan completed. Discovered %d devices.", (int)loadedEntities.size());
        int dIdx = 0;
        for (const auto& entity : loadedEntities) {
            ESP_LOGI(TAG, "Discovered Device #%d:", dIdx++);
            ESP_LOGI(TAG, "  Name         : %s", entity.displayName.c_str());
            ESP_LOGI(TAG, "  Manufacturer : %s", entity.identity.vendor.c_str());
            ESP_LOGI(TAG, "  Model        : %s", entity.identity.model.c_str());
            ESP_LOGI(TAG, "  PrimaryClass : %s", NetDiscovery::ToString(entity.primaryClass).c_str());
            ESP_LOGI(TAG, "  IP Address   : %s", !entity.endpoints.empty() ? entity.endpoints[0].ip.c_str() : "None");
            ESP_LOGI(TAG, "  Capabilities :");
            for (const auto& cap : entity.capabilities.GetCapabilities()) {
                ESP_LOGI(TAG, "    - %s", NetDiscovery::ToString(cap).c_str());
            }
            ESP_LOGI(TAG, "  Controllers  :");
            for (const auto& ctrl : entity.compatibleControllers) {
                ESP_LOGI(TAG, "    - %s (Score: %d)", ctrl.name.c_str(), ctrl.confidence);
            }
            ESP_LOGI(TAG, "-----------------------------------------------");
        }
        // Purge raw SSDP/mDNS evidence vector from RAM before audio ignition
        registry.Clear();
        ESP_LOGI(TAG, "🧹 [DeviceRegistry] Purged raw evidence vector prior to audio ignition");

        // Free SSDP socket resources since discovery is a one-time operation
        g_ssdpClient->Shutdown();
        g_ssdpClient.reset();

        orchestrator_post_event(ORCH_EVENT_NETDISCOVERY_COMPLETE);
    };
    
    // Spawn scan in a thread to not block boot sequence (Priority 2 to not starve WebRTC Audio)
    // Fix 3: stack lives in PSRAM; LittleFS mutations are delegated to the nd_store_writer
    // task (internal-RAM stack) so this task never triggers a flash-op cache-disable window.
    if (!NetDiscovery::ThreadHelper::StartPinnedThread("nd_oneshot", 16384, 2, 0, scan_lambda)) {
        ESP_LOGE(TAG, "nd_oneshot task creation failed, aborting scan timer");
        return false;
    }
    return true;
}

static uint16_t parse_port_from_url(const std::string& url) {
    size_t pos = url.find("://");
    size_t start = (pos != std::string::npos) ? pos + 3 : 0;
    size_t colon = url.find(':', start);
    if (colon != std::string::npos) {
        size_t slash = url.find('/', colon);
        std::string port_str = (slash != std::string::npos) 
            ? url.substr(colon + 1, slash - colon - 1) 
            : url.substr(colon + 1);
        int p = std::atoi(port_str.c_str());
        if (p > 0 && p <= 65535) return (uint16_t)p;
    }
    return 0;
}

bool netdiscovery_get_entity_endpoint(const char* target_name, char* out_ip, size_t ip_len, uint32_t* out_port, bool* out_trusted) {
    if (!g_knowledgeStore || !target_name || !out_ip || !out_port) return false;
    out_ip[0] = '\0';
    *out_port = 8080;
    if (out_trusted) *out_trusted = false;

    auto entities = g_knowledgeStore->GetLoadedEntities();
    if (entities.empty()) {
        ESP_LOGW(TAG, "[netdiscovery_get_entity_endpoint] No entities loaded in KnowledgeStore");
        return false;
    }

    std::string targetLower = target_name;
    for (char &c : targetLower) c = (char)std::tolower((unsigned char)c);

    const NetDiscovery::KnowledgeEntity* matchedEntity = nullptr;

    // Pass 1: Exact match (case-insensitive) against persistentId, displayName, or aliases
    for (const auto& entity : entities) {
        std::string nameLower = entity.displayName;
        for (char &c : nameLower) c = (char)std::tolower((unsigned char)c);
        std::string idLower = entity.persistentId;
        for (char &c : idLower) c = (char)std::tolower((unsigned char)c);

        if (nameLower == targetLower || idLower == targetLower) {
            matchedEntity = &entity;
            break;
        }
        for (const auto& alias : entity.aliases.userAliases) {
            std::string aLower = alias;
            for (char &c : aLower) c = (char)std::tolower((unsigned char)c);
            if (aLower == targetLower) { matchedEntity = &entity; break; }
        }
        if (matchedEntity) break;
    }

    // Pass 2: Substring / Fuzzy match (case-insensitive)
    if (!matchedEntity) {
        for (const auto& entity : entities) {
            std::string nameLower = entity.displayName;
            for (char &c : nameLower) c = (char)std::tolower((unsigned char)c);
            std::string idLower = entity.persistentId;
            for (char &c : idLower) c = (char)std::tolower((unsigned char)c);

            if ((!nameLower.empty() && (nameLower.find(targetLower) != std::string::npos || targetLower.find(nameLower) != std::string::npos)) ||
                (!idLower.empty() && (idLower.find(targetLower) != std::string::npos || targetLower.find(idLower) != std::string::npos))) {
                matchedEntity = &entity;
                break;
            }
        }
    }

    // Pass 3: Fallback to first SmartTV
    if (!matchedEntity) {
        for (const auto& entity : entities) {
            if (entity.primaryClass == NetDiscovery::PrimaryDeviceClass::SmartTV) {
                matchedEntity = &entity;
                ESP_LOGI(TAG, "[netdiscovery_get_entity_endpoint] Target '%s' not matched by name; fallback to SmartTV '%s'",
                         target_name, entity.displayName.c_str());
                break;
            }
        }
    }

    if (!matchedEntity) {
        ESP_LOGW(TAG, "[netdiscovery_get_entity_endpoint] No matching entity or SmartTV fallback found for '%s'", target_name);
        return false;
    }

    // Extract IP address
    std::string ipStr;
    if (!matchedEntity->endpoints.empty() && !matchedEntity->endpoints[0].ip.empty()) {
        ipStr = matchedEntity->endpoints[0].ip;
    } else if (!matchedEntity->runtimeState.endpoints.empty() && !matchedEntity->runtimeState.endpoints[0].ip.empty()) {
        ipStr = matchedEntity->runtimeState.endpoints[0].ip;
    }

    if (ipStr.empty()) {
        ESP_LOGW(TAG, "[netdiscovery_get_entity_endpoint] Entity '%s' has no IP endpoint", matchedEntity->displayName.c_str());
        return false;
    }

    snprintf(out_ip, ip_len, "%s", ipStr.c_str());

    // Extract Port dynamically
    uint16_t resolvedPort = 0;

    for (const auto& s : matchedEntity->services) {
        if (!s.controlUrl.empty()) resolvedPort = parse_port_from_url(s.controlUrl);
        if (resolvedPort != 0) break;
        if (!s.eventUrl.empty()) resolvedPort = parse_port_from_url(s.eventUrl);
        if (resolvedPort != 0) break;
    }
    if (resolvedPort == 0) {
        for (const auto& ns : matchedEntity->normalizedServices) {
            if (!ns.endpointUrl.empty()) resolvedPort = parse_port_from_url(ns.endpointUrl);
            if (resolvedPort != 0) break;
        }
    }

    // Brand-specific port heuristics if no explicit port found in service URLs
    if (resolvedPort == 0) {
        std::string vendor = matchedEntity->identity.vendor;
        std::string disp = matchedEntity->displayName;
        for (char &c : vendor) c = (char)std::tolower((unsigned char)c);
        for (char &c : disp) c = (char)std::tolower((unsigned char)c);

        if (vendor.find("roku") != std::string::npos || disp.find("roku") != std::string::npos) {
            resolvedPort = 8060;
        } else if (vendor.find("lg") != std::string::npos || disp.find("webos") != std::string::npos) {
            resolvedPort = 3000;
        } else if (vendor.find("samsung") != std::string::npos || disp.find("samsung") != std::string::npos) {
            resolvedPort = 8080;
        } else {
            resolvedPort = 8080; // General DIAL default
        }
    }

    *out_port = resolvedPort;

    if (out_trusted) {
        *out_trusted = false;
        if (g_controllerRegistry && matchedEntity) {
            for (const auto& ctrl : g_controllerRegistry->GetControllers()) {
                if (ctrl && ctrl->ReachabilityTrust() == NetDiscovery::PowerStateReachabilityTrust::Confirmed) {
                    std::string vendorLower = matchedEntity->identity.vendor;
                    for (char &c : vendorLower) c = (char)std::tolower((unsigned char)c);
                    for (const auto& mfg : ctrl->SupportedManufacturers()) {
                        std::string mfgLower = mfg;
                        for (char &c : mfgLower) c = (char)std::tolower((unsigned char)c);
                        if (!mfgLower.empty() && !vendorLower.empty() &&
                            (vendorLower.find(mfgLower) != std::string::npos || mfgLower.find(vendorLower) != std::string::npos)) {
                            *out_trusted = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    ESP_LOGI(TAG, "[netdiscovery_get_entity_endpoint] Resolved target '%s' -> Entity '%s' (%s:%u) trust=%s",
             target_name, matchedEntity->displayName.c_str(), out_ip, (unsigned)*out_port,
             (out_trusted && *out_trusted) ? "CONFIRMED" : "UNCONFIRMED");
    return true;
}

