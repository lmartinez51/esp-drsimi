#include "esp_claw_init.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdio.h>
#include "cJSON.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "esp_littlefs.h"
#include "lua_ir_bindings.h"
#include "lua_mic_bindings.h"
#include "app_events.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "webrtc.h"
#include "NetDiscoveryIPC.h"
#include "../../../components/net_discovery/include/NetDiscoveryMetrics.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>

static const char *TAG = "ESP_CLAW_ISO";
static QueueHandle_t s_test_queue = NULL;
static QueueHandle_t s_lua_to_c_queue = NULL;
static EventGroupHandle_t s_claw_event_group = NULL;

#define LUA_SAFE_TO_START_BIT BIT0
#define LUA_ENGINE_READY_BIT BIT1

static volatile bool g_fs_corrupted = false;

bool esp_claw_is_automation_ready(void) {
    if (!s_claw_event_group) return false;
    return (xEventGroupGetBits(s_claw_event_group) & LUA_ENGINE_READY_BIT) != 0;
}

bool esp_claw_is_fs_corrupted(void) {
    return g_fs_corrupted;
}

void esp_claw_signal_safe_to_start(void) {
    if (s_claw_event_group) {
        xEventGroupSetBits(s_claw_event_group, LUA_SAFE_TO_START_BIT);
    }
}

static void* claw_lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;  (void)osize;
    if (nsize == 0) {
        heap_caps_free(ptr);
        return NULL;
    } else {
        return heap_caps_realloc(ptr, nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
}

static void claw_push_rule_to_lua(lua_State *L, esp_claw_rule_t *rule) {
    lua_newtable(L);
    
    lua_pushstring(L, rule->call_id);
    lua_setfield(L, -2, "call_id");
    
    lua_pushstring(L, rule->trigger);
    lua_setfield(L, -2, "trigger");
    
    lua_newtable(L);
    for (int i = 0; i < rule->num_conditions; i++) {
        lua_newtable(L); 
        
        lua_pushstring(L, rule->conditions[i].sensor);
        lua_setfield(L, -2, "sensor");
        
        lua_pushstring(L, rule->conditions[i].op);
        lua_setfield(L, -2, "op");
        
        if (rule->conditions[i].val_type == VAL_TYPE_NUMBER) {
            lua_pushnumber(L, rule->conditions[i].f_val);
        } else if (rule->conditions[i].val_type == VAL_TYPE_BOOL) {
            lua_pushboolean(L, rule->conditions[i].b_val);
        } else {
            lua_pushstring(L, rule->conditions[i].s_val);
        }
        lua_setfield(L, -2, "val");
        
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "conditions");
    
    lua_newtable(L);
    for (int i = 0; i < rule->num_actions; i++) {
        lua_pushstring(L, rule->actions[i].target);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "actions");
}

static int l_sys_delay(lua_State *L) {
    if (lua_gettop(L) >= 1) {
        int ms = lua_tointeger(L, 1);
        if (ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(ms));
        }
    }
    return 0;
}

static int l_send_response(lua_State *L) {
    if (lua_gettop(L) >= 1) {
        const char* msg = lua_tostring(L, 1);
        if (msg && s_lua_to_c_queue) {
            esp_claw_response_t resp;
            resp.success = true;
            strlcpy(resp.payload, msg, sizeof(resp.payload));
            xQueueSend(s_lua_to_c_queue, &resp, 0);
        }
    }
    return 0;
}

extern int send_function_output(const char *call_id, const char *output);
extern int sendEvent(const char *type, const char *payload);

static int l_send_webrtc_response(lua_State *L) {
    if (lua_gettop(L) >= 2) {
        const char* call_id = lua_tostring(L, 1);
        const char* payload = lua_tostring(L, 2);
        if (call_id && payload) {
            send_function_output(call_id, payload);
            sendEvent("response.create", NULL);
        }
    }
    return 0;
}

static uint32_t s_next_request_id = 1000;

static int l_send_intent(lua_State *L) {
    int top = lua_gettop(L);
    if (top >= 3) {
        const char* call_id         = lua_tostring(L, 1);
        const char* action          = lua_tostring(L, 2);
        const char* target          = lua_tostring(L, 3);
        const char* room            = (top >= 4 && lua_tostring(L, 4)) ? lua_tostring(L, 4) : "";
        const char* entity_id       = (top >= 5 && lua_tostring(L, 5)) ? lua_tostring(L, 5) : "";
        const char* parameters_json = (top >= 6 && lua_tostring(L, 6)) ? lua_tostring(L, 6) : "{}";

        if (call_id && action && target && netdiscovery_intent_queue) {
            netdiscovery_intent_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.version = NETDISCOVERY_IPC_VERSION;
            msg.request_id = __atomic_fetch_add(&s_next_request_id, 1, __ATOMIC_RELAXED);

            strlcpy(msg.call_id, call_id, sizeof(msg.call_id));
            strlcpy(msg.action, action, sizeof(msg.action));
            strlcpy(msg.target, target, sizeof(msg.target));
            strlcpy(msg.room, room, sizeof(msg.room));
            strlcpy(msg.entity_id, entity_id, sizeof(msg.entity_id));
            strlcpy(msg.parameters_json, parameters_json, sizeof(msg.parameters_json));

            netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "netdiscovery_intent_t created in C-binding");

            ESP_LOGI(TAG, "[%u][%s] C-binding posting intent: action='%s', target='%s', parameters_json='%s'",
                     (unsigned)msg.request_id, msg.call_id, msg.action, msg.target, msg.parameters_json);

            netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "netdiscovery_intent_t pushing by value to queue");
            if (xQueueSend(netdiscovery_intent_queue, &msg, 0) != pdTRUE) {
                ESP_LOGE(TAG, "[%u][%s] NetDiscovery IPC queue full", (unsigned)msg.request_id, msg.call_id);
                netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "queue push failed - netdiscovery_intent_t destroyed");
                send_function_output(call_id, "{\"error\": \"System Busy\"}");
                sendEvent("response.create", NULL);
            } else {
                netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "netdiscovery_intent_t successfully queued");
            }
        } else {
            ESP_LOGW(TAG, "[ESP_CLAW] c_send_intent missing required non-null parameters");
        }
    } else {
        ESP_LOGW(TAG, "[ESP_CLAW] c_send_intent requires at least 3 arguments (call_id, action, target)");
    }
    return 0;
}

static int l_inject_webrtc_message(lua_State *L) {
    if (lua_gettop(L) >= 1) {
        const char* message = lua_tostring(L, 1);
        if (message) {
            if (webrtc_is_server_generating()) {
                sendEvent("response.cancel", NULL);
            }
            sendEvent("conversation.item.create", message);
            sendEvent("response.create", NULL);
        }
    }
    return 0;
}

static bool is_tcp_port_open(const char *ip_str, uint16_t port, uint32_t timeout_ms) {
    if (!ip_str || port == 0) return false;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_str, &addr.sin_addr) <= 0) {
        close(fd);
        return false;
    }

    int res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (res == 0) {
        close(fd);
        return true;
    }

    if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
        close(fd);
        return false;
    }

    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    res = select(fd + 1, NULL, &writefds, NULL, &tv);
    if (res > 0 && FD_ISSET(fd, &writefds)) {
        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
            close(fd);
            return true;
        }
    }

    close(fd);
    return false;
}

static int l_is_endpoint_ready(lua_State *L) {
    const char *ip = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);
    int timeout_ms = 300;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
        timeout_ms = lua_tointeger(L, 3);
    }
    bool ready = is_tcp_port_open(ip, (uint16_t)port, (uint32_t)timeout_ms);
    ESP_LOGI(TAG, "[REACHABILITY PROBE] ip=%s port=%d result=%s",
             ip, port, ready ? "READY" : "UNREACHABLE");
    lua_pushboolean(L, ready);
    return 1;
}

static int l_get_entity_endpoint(lua_State *L) {
    const char *target = luaL_checkstring(L, 1);
    char ip[64] = {0};
    uint32_t port = 8080;
    bool is_trusted = false;
    bool found = netdiscovery_get_entity_endpoint(target, ip, sizeof(ip), &port, &is_trusted);
    if (found) {
        lua_pushstring(L, ip);
        lua_pushinteger(L, (lua_Integer)port);
        lua_pushboolean(L, is_trusted);
        return 3;
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushboolean(L, false);
        return 3;
    }
}

// RESTAURADO: Hooks originales para mandar el JSON a PSRAM y salvar la RAM interna
static void* cjson_spiram_malloc(size_t sz) { return heap_caps_malloc(sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
static void cjson_spiram_free(void* ptr) { heap_caps_free(ptr); }

static int l_save_rules_to_fs(lua_State *L) {
    cJSON_Hooks hooks = { .malloc_fn = cjson_spiram_malloc, .free_fn = cjson_spiram_free };
    cJSON_InitHooks(&hooks);

    cJSON *root_array = cJSON_CreateArray();
    if (!root_array) {
        cJSON_InitHooks(NULL);
        return 0;
    }

    lua_getglobal(L, "rules_db");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            cJSON *rule_json = cJSON_CreateObject();
            
            lua_getfield(L, -1, "trigger");
            const char* trigger_str = lua_tostring(L, -1);
            cJSON_AddStringToObject(rule_json, "trigger", trigger_str ? trigger_str : "");
            lua_pop(L, 1);
            
            cJSON *conditions_array = cJSON_AddArrayToObject(rule_json, "conditions");
            lua_getfield(L, -1, "conditions");
            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    cJSON *cond_json = cJSON_CreateObject();
                    lua_getfield(L, -1, "sensor"); 
                    const char* sensor_str = lua_tostring(L, -1);
                    cJSON_AddStringToObject(cond_json, "sensor", sensor_str ? sensor_str : ""); 
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "op"); 
                    const char* op_str = lua_tostring(L, -1);
                    cJSON_AddStringToObject(cond_json, "op", op_str ? op_str : ""); 
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "val");
                    if (lua_type(L, -1) == LUA_TNUMBER) {
                        cJSON_AddNumberToObject(cond_json, "val", lua_tonumber(L, -1));
                    } else if (lua_type(L, -1) == LUA_TBOOLEAN) {
                        cJSON_AddBoolToObject(cond_json, "val", lua_toboolean(L, -1));
                    } else {
                        const char* val_str = lua_tostring(L, -1);
                        cJSON_AddStringToObject(cond_json, "val", val_str ? val_str : "");
                    }
                    lua_pop(L, 1);
                    cJSON_AddItemToArray(conditions_array, cond_json);
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
            
            cJSON *actions_array = cJSON_AddArrayToObject(rule_json, "actions");
            lua_getfield(L, -1, "actions");
            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    const char* act_str = lua_tostring(L, -1);
                    cJSON_AddItemToArray(actions_array, cJSON_CreateString(act_str ? act_str : ""));
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
            
            cJSON_AddItemToArray(root_array, rule_json);
            lua_pop(L, 1); 
        }
    }
    lua_pop(L, 1); 

    char *psram_json_str = cJSON_PrintUnformatted(root_array);
    if (psram_json_str) {
        size_t len = strlen(psram_json_str);
        char *internal_json_buffer = heap_caps_malloc(len + 1, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (internal_json_buffer) {
            strcpy(internal_json_buffer, psram_json_str);
            FILE *f = fopen("/littlefs/rules.json", "w");
            if (f) {
                fputs(internal_json_buffer, f);
                fsync(fileno(f));
                fclose(f);
                ESP_LOGI(TAG, "Rules securely saved to LittleFS via Internal SRAM.");
            } else {
                ESP_LOGE(TAG, "Failed to write rules to file.");
            }
            heap_caps_free(internal_json_buffer);
        } else {
            ESP_LOGE(TAG, "Failed to allocate internal RAM for rule save.");
        }
        cjson_spiram_free(psram_json_str);
    }
    
    cJSON_Delete(root_array);
    cJSON_InitHooks(NULL);
    return 0;
}

static void esp_claw_load_rules_from_fs(lua_State *L) {
    FILE *f = fopen("/littlefs/rules.json", "r");
    if (!f) {
        ESP_LOGI(TAG, "No rules.json found (first boot or wiped).");
        return;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(f);
        return;
    }

    char *json_str = heap_caps_malloc(fsize + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for deserialization.");
        fclose(f);
        return;
    }

    size_t read_bytes = fread(json_str, 1, fsize, f);
    json_str[read_bytes] = '\0';
    fclose(f);

    cJSON_Hooks hooks = { .malloc_fn = cjson_spiram_malloc, .free_fn = cjson_spiram_free };
    cJSON_InitHooks(&hooks);

    cJSON *root = cJSON_Parse(json_str);
    heap_caps_free(json_str);

    if (root && cJSON_IsArray(root)) {
        lua_getglobal(L, "rules_db");
        if (lua_istable(L, -1)) {
            cJSON *rule_item;
            cJSON_ArrayForEach(rule_item, root) {
                cJSON *trigger = cJSON_GetObjectItem(rule_item, "trigger");
                if (!cJSON_IsString(trigger)) continue;

                lua_pushstring(L, trigger->valuestring); 

                lua_newtable(L); 
                
                lua_pushstring(L, trigger->valuestring);
                lua_setfield(L, -2, "trigger");
                
                cJSON *conditions = cJSON_GetObjectItem(rule_item, "conditions");
                if (cJSON_IsArray(conditions)) {
                    lua_newtable(L);
                    int cond_idx = 1;
                    cJSON *cond_item;
                    cJSON_ArrayForEach(cond_item, conditions) {
                        lua_pushinteger(L, cond_idx++);
                        lua_newtable(L);
                        
                        cJSON *sensor = cJSON_GetObjectItem(cond_item, "sensor");
                        if (cJSON_IsString(sensor)) { lua_pushstring(L, sensor->valuestring); lua_setfield(L, -2, "sensor"); }
                        
                        cJSON *op = cJSON_GetObjectItem(cond_item, "op");
                        if (cJSON_IsString(op)) { lua_pushstring(L, op->valuestring); lua_setfield(L, -2, "op"); }
                        
                        cJSON *val = cJSON_GetObjectItem(cond_item, "val");
                        if (cJSON_IsNumber(val)) { lua_pushnumber(L, val->valuedouble); lua_setfield(L, -2, "val"); }
                        else if (cJSON_IsBool(val)) { lua_pushboolean(L, cJSON_IsTrue(val)); lua_setfield(L, -2, "val"); }
                        else if (cJSON_IsString(val)) { lua_pushstring(L, val->valuestring); lua_setfield(L, -2, "val"); }
                        
                        lua_settable(L, -3); 
                    }
                    lua_setfield(L, -2, "conditions");
                }
                
                cJSON *actions = cJSON_GetObjectItem(rule_item, "actions");
                if (cJSON_IsArray(actions)) {
                    lua_newtable(L);
                    int act_idx = 1;
                    cJSON *act_item;
                    cJSON_ArrayForEach(act_item, actions) {
                        if (cJSON_IsString(act_item)) {
                            lua_pushinteger(L, act_idx++);
                            lua_pushstring(L, act_item->valuestring);
                            lua_settable(L, -3); 
                        }
                    }
                    lua_setfield(L, -2, "actions");
                }
                
                lua_settable(L, -3); 
            }
        }
        lua_pop(L, 1); 
        ESP_LOGI(TAG, "Rules securely loaded from LittleFS and injected into Lua rules_db.");
    }
    if (!root || !cJSON_IsArray(root)) {
        ESP_LOGE(TAG, "Failed to parse rules.json.");
    }
    
    if (root) cJSON_Delete(root);
    cJSON_InitHooks(NULL);
}

static void instruction_limit_hook(lua_State *L, lua_Debug *ar) {
    luaL_error(L, "Execution limit exceeded. Rule Engine killed infinite loop!");
}

static bool esp_claw_lua_has_register_rule(lua_State *L) {
    lua_getglobal(L, "register_rule");
    bool present = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return present;
}

static int c_message_handler(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) { 
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING) return 1;
        else msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);
    return 1;
}

static void lua_worker_task(void *arg) {
    ESP_LOGI(TAG, "Starting Lua Isolation Test Worker");
    
    lua_State *L = lua_newstate(claw_lua_alloc, NULL, 0);
    if (!L) {
        ESP_LOGE(TAG, "lua_newstate failed.");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Loading approved standard libraries (PSRAM)");
    luaL_requiref(L, "_G", luaopen_base, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L, 1);
    
    lua_register(L, "c_sys_delay", l_sys_delay);
    lua_register(L, "c_send_response", l_send_response);
    lua_register(L, "c_send_webrtc_response", l_send_webrtc_response);
    lua_register(L, "c_inject_webrtc_message", l_inject_webrtc_message);
    lua_register(L, "c_save_rules", l_save_rules_to_fs);
    lua_register(L, "c_send_intent", l_send_intent);
    lua_register(L, "c_is_endpoint_ready", l_is_endpoint_ready);
    lua_register(L, "c_get_entity_endpoint", l_get_entity_endpoint);

    ESP_LOGI(TAG, "Registering IR bindings safely");
    luaL_requiref(L, "ir", luaopen_ir, 1);
    lua_pop(L, 1);
    ESP_LOGI(TAG, "IR bindings registered successfully");
    ESP_LOGI(TAG, "Lua initialized. Waiting for LUA_SAFE_TO_START_BIT...");
    xEventGroupWaitBits(s_claw_event_group, LUA_SAFE_TO_START_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    ESP_LOGI(TAG, "LUA_SAFE_TO_START_BIT received. Safe to access SPI Flash.");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "littlefs",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };
    if (esp_vfs_littlefs_register(&conf) != ESP_OK) {
        ESP_LOGE(TAG, "LittleFS mount FAILED. Entering RAM-only degraded mode. "
                      "IR DB and Rules DB are OFFLINE. "
                      "Long-press MUTE for 10s to factory reset.");
        g_fs_corrupted = true;
    } else {
        if (mkdir("/littlefs/netdiscovery", 0777) == -1 && errno != EEXIST) {
            ESP_LOGE(TAG, "Failed to create /littlefs/netdiscovery directory: %s", strerror(errno));
        }

        bool init_ok = (luaL_dofile(L, "/littlefs/init.lua") == LUA_OK);
        if (!init_ok) {
            ESP_LOGE(TAG, "Failed to load init.lua: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        } else if (!esp_claw_lua_has_register_rule(L)) {
            ESP_LOGE(TAG,
                "init.lua executed but register_rule() is undefined. "
                "Script on flash is stale or incompatible. "
                "Automation engine will NOT come online. "
                "Reflash data/init.lua to restore functionality.");
            init_ok = false;
        }

        if (init_ok) {
            ESP_LOGI(TAG, "Successfully executed init.lua");
            esp_claw_load_rules_from_fs(L);
            xEventGroupSetBits(s_claw_event_group, LUA_ENGINE_READY_BIT);
        }
    }

    ESP_LOGI(TAG, "Entering isolated infinite IPC loop");
    esp_claw_rule_t* msg_rule = NULL;
    while (1) {
        if (xQueueReceive(s_test_queue, &msg_rule, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received new rule pointer. Mapping to Lua table...");
            
            if (msg_rule != NULL) {
                esp_claw_rule_t* current = msg_rule;
                while (current != NULL) {
                    esp_claw_rule_t* next = current->next;

                    lua_pushcfunction(L, c_message_handler);   
                    int handler_idx = lua_gettop(L);

                    lua_getglobal(L, "register_rule");          
                    
                    if (lua_isfunction(L, -1)) {
                        claw_push_rule_to_lua(L, current);      
                        
                        lua_sethook(L, instruction_limit_hook, LUA_MASKCOUNT, 50000);
                        if (lua_pcall(L, 1, 0, handler_idx) != LUA_OK) {
                            ESP_LOGE(TAG, "Lua Fatal Error:\n%s", lua_tostring(L, -1));
                            lua_pop(L, 1);
                            if (current->call_id[0] != '\0') {
                                send_function_output(current->call_id, "{\"error\":\"rule execution failed\"}");
                                sendEvent("response.create", NULL);
                            }
                        }
                        lua_sethook(L, instruction_limit_hook, 0, 0);
                    } else {
                        lua_pop(L, 1); 
                        ESP_LOGE(TAG, "register_rule function not found in Lua environment");
                        if (current->call_id[0] != '\0') {
                            send_function_output(current->call_id, "{\"error\":\"automation engine unavailable\"}");
                            sendEvent("response.create", NULL);
                        }
                    }
                    lua_pop(L, 1); 
                    
                    free(current);
                    current = next;
                }
            }
            
            lua_gc(L, LUA_GCSTEP, 0);
        }
    }
}

esp_err_t esp_claw_init(void) {
    ESP_LOGI(TAG, "Creating IPC test queue");
    s_test_queue = xQueueCreate(15, sizeof(esp_claw_rule_t*));
    if (!s_test_queue) {
        ESP_LOGE(TAG, "Failed to create s_test_queue");
        return ESP_ERR_NO_MEM;
    }
    
    s_lua_to_c_queue = xQueueCreate(5, sizeof(esp_claw_response_t));
    if (!s_lua_to_c_queue) {
        ESP_LOGE(TAG, "Failed to create s_lua_to_c_queue");
        return ESP_ERR_NO_MEM;
    }

    s_claw_event_group = xEventGroupCreate();
    if (!s_claw_event_group) {
        ESP_LOGE(TAG, "Failed to create s_claw_event_group");
        return ESP_ERR_NO_MEM;
    }

    // RESTAURADO: El stack vuelve estrictamente a sus 16KB originales
    ESP_LOGI(TAG, "Spawning isolated Lua worker task (Internal SRAM - 16KB)");
    if (xTaskCreatePinnedToCoreWithCaps(lua_worker_task, "lua_worker", 16384, NULL, 3, NULL, 1, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) != pdPASS) {
        ESP_LOGE(TAG, "Failed to spawn lua_worker_task.");
        return ESP_FAIL;
    }

    netdiscovery_ipc_init();

    return ESP_OK;
}

esp_err_t esp_claw_send_command(const char* device, const char* action) {
    if (s_test_queue == NULL) return ESP_ERR_INVALID_STATE;
    ESP_LOGW(TAG, "esp_claw_send_command is temporarily disabled during JSON Rule Engine migration.");
    return ESP_OK;
}

esp_err_t esp_claw_send_rule(esp_claw_rule_t* rule) {
    if (s_test_queue == NULL) return ESP_ERR_INVALID_STATE;
    if (xQueueSend(s_test_queue, &rule, 0) != pdTRUE) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t esp_claw_request_list(char* out_buffer, size_t max_len) {
    if (s_test_queue == NULL || s_lua_to_c_queue == NULL) return ESP_ERR_INVALID_STATE;
    
    xQueueReset(s_lua_to_c_queue);

    esp_claw_rule_t* req = malloc(sizeof(esp_claw_rule_t));
    if (!req) return ESP_ERR_NO_MEM;
    
    memset(req, 0, sizeof(esp_claw_rule_t));
    strlcpy(req->trigger, "SYS_CMD:LIST", sizeof(req->trigger));
    
    if (xQueueSend(s_test_queue, &req, pdMS_TO_TICKS(100)) != pdTRUE) {
        free(req);
        strlcpy(out_buffer, "Error: Queue is full.", max_len);
        return ESP_FAIL;
    }
    
    esp_claw_response_t resp;
    if (xQueueReceive(s_lua_to_c_queue, &resp, pdMS_TO_TICKS(1500)) == pdTRUE) {
        snprintf(out_buffer, max_len, "%s", resp.payload);
        return resp.success ? ESP_OK : ESP_FAIL;
    } else {
        strlcpy(out_buffer, "Error: Hardware engine is currently busy executing a macro. Try again in a few seconds.", max_len);
        return ESP_FAIL;
    }
}

esp_err_t esp_claw_request_delete(const char* trigger, char* out_buffer, size_t max_len) {
    if (s_test_queue == NULL || s_lua_to_c_queue == NULL) return ESP_ERR_INVALID_STATE;
    
    xQueueReset(s_lua_to_c_queue);

    esp_claw_rule_t* req = malloc(sizeof(esp_claw_rule_t));
    if (!req) return ESP_ERR_NO_MEM;
    
    memset(req, 0, sizeof(esp_claw_rule_t));
    strlcpy(req->trigger, "SYS_CMD:DELETE", sizeof(req->trigger));
    
    req->num_actions = 1;
    strlcpy(req->actions[0].target, trigger, sizeof(req->actions[0].target));
    req->actions[0].target[sizeof(req->actions[0].target)-1] = '\0';
    
    if (xQueueSend(s_test_queue, &req, pdMS_TO_TICKS(100)) != pdTRUE) {
        free(req);
        strlcpy(out_buffer, "Error: Queue is full.", max_len);
        return ESP_FAIL;
    }
    
    esp_claw_response_t resp;
    if (xQueueReceive(s_lua_to_c_queue, &resp, pdMS_TO_TICKS(1500)) == pdTRUE) {
        snprintf(out_buffer, max_len, "%s", resp.payload);
        return resp.success ? ESP_OK : ESP_FAIL;
    } else {
        strlcpy(out_buffer, "Error: Hardware engine is currently busy executing a macro. Try again in a few seconds.", max_len);
        return ESP_FAIL;
    }
}