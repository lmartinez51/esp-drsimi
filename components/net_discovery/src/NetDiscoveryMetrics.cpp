#include "../include/NetDiscoveryMetrics.h"
#include "../include/NetDiscoveryIPC.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static const char* TAG = "NetDiscoveryMetrics";

static uint32_t s_max_claw_q_depth = 0;
static uint32_t s_max_netdisc_q_depth = 0;

void netdiscovery_get_memory_snapshot(netdiscovery_memory_snapshot_t* snapshot) {
    if (!snapshot) return;
    snapshot->internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    snapshot->internal_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    snapshot->psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    snapshot->psram_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void netdiscovery_print_memory_report(const char* label, const netdiscovery_memory_snapshot_t* before, const netdiscovery_memory_snapshot_t* after) {
    if (!before || !after) return;
    int internal_delta = (int)after->internal_free - (int)before->internal_free;
    int psram_delta    = (int)after->psram_free - (int)before->psram_free;

    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "Memory Report [%s]", label ? label : "Tool Call");
    ESP_LOGI(TAG, "Internal Free Before : %u bytes | After: %u bytes (Delta: %d)", (unsigned)before->internal_free, (unsigned)after->internal_free, internal_delta);
    ESP_LOGI(TAG, "Internal Max Block   : %u bytes | After: %u bytes", (unsigned)before->internal_largest_block, (unsigned)after->internal_largest_block);
    ESP_LOGI(TAG, "PSRAM Free Before    : %u bytes | After: %u bytes (Delta: %d)", (unsigned)before->psram_free, (unsigned)after->psram_free, psram_delta);
    ESP_LOGI(TAG, "PSRAM Max Block      : %u bytes | After: %u bytes", (unsigned)before->psram_largest_block, (unsigned)after->psram_largest_block);
    ESP_LOGI(TAG, "========================================================");
}

void netdiscovery_print_latency_report(uint32_t request_id, const char* call_id, const netdiscovery_pipeline_timestamps_t* ts) {
    if (!ts) return;
    int64_t webrtc_to_claw = (ts->t1_claw_enqueue > ts->t0_webrtc_recv) ? (ts->t1_claw_enqueue - ts->t0_webrtc_recv) / 1000 : 0;
    int64_t claw_to_lua    = (ts->t2_lua_recv > ts->t1_claw_enqueue)     ? (ts->t2_lua_recv - ts->t1_claw_enqueue) / 1000 : 0;
    int64_t lua_to_ipc     = (ts->t3_ipc_push > ts->t2_lua_recv)         ? (ts->t3_ipc_push - ts->t2_lua_recv) / 1000 : 0;
    int64_t ipc_pop_delay  = (ts->t4_ipc_pop > ts->t3_ipc_push)          ? (ts->t4_ipc_pop - ts->t3_ipc_push) / 1000 : 0;
    int64_t validator_time = (ts->t5_validator_done > ts->t4_ipc_pop)   ? (ts->t5_validator_done - ts->t4_ipc_pop) / 1000 : 0;
    int64_t semantic_time  = (ts->t6_semantic_done > ts->t5_validator_done) ? (ts->t6_semantic_done - ts->t5_validator_done) / 1000 : 0;
    int64_t response_time  = (ts->t7_response_sent > ts->t6_semantic_done)  ? (ts->t7_response_sent - ts->t6_semantic_done) / 1000 : 0;
    int64_t total_pipeline = (ts->t7_response_sent > ts->t0_webrtc_recv)    ? (ts->t7_response_sent - ts->t0_webrtc_recv) / 1000 : 0;

    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "Pipeline Latency Report [%u][%s]", (unsigned)request_id, call_id ? call_id : "N/A");
    ESP_LOGI(TAG, "WebRTC -> Claw Enqueue   : %lld ms", (long long)webrtc_to_claw);
    ESP_LOGI(TAG, "Claw -> Lua Receive      : %lld ms", (long long)claw_to_lua);
    ESP_LOGI(TAG, "Lua -> IPC Push          : %lld ms", (long long)lua_to_ipc);
    ESP_LOGI(TAG, "IPC Queue Pop Delay      : %lld ms", (long long)ipc_pop_delay);
    ESP_LOGI(TAG, "Validator Execution      : %lld ms", (long long)validator_time);
    ESP_LOGI(TAG, "Semantic Stub Resolution : %lld ms", (long long)semantic_time);
    ESP_LOGI(TAG, "Response Output Dispatch : %lld ms", (long long)response_time);
    ESP_LOGI(TAG, "TOTAL PIPELINE LATENCY   : %lld ms", (long long)total_pipeline);
    ESP_LOGI(TAG, "========================================================");

    if (total_pipeline > 50) {
        ESP_LOGW(TAG, "[AUDIO SAFETY WARNING] Total pipeline latency (%lld ms) exceeded 50ms threshold for request_id=%u", (long long)total_pipeline, (unsigned)request_id);
    }
}

void netdiscovery_print_stack_report(TaskHandle_t claw_task, TaskHandle_t ipc_task) {
    UBaseType_t claw_stack = claw_task ? uxTaskGetStackHighWaterMark(claw_task) * sizeof(StackType_t) : 0;
    UBaseType_t ipc_stack  = ipc_task ? uxTaskGetStackHighWaterMark(ipc_task) * sizeof(StackType_t) : 0;
    UBaseType_t curr_stack = uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);

    ESP_LOGI(TAG, "Stack High Water Mark Report:");
    if (claw_task) ESP_LOGI(TAG, "  ESP-Claw Task Stack Free  : %u bytes", (unsigned)claw_stack);
    if (ipc_task)  ESP_LOGI(TAG, "  IPC Worker Task Stack Free : %u bytes", (unsigned)ipc_stack);
    ESP_LOGI(TAG, "  Current Task Stack Free    : %u bytes", (unsigned)curr_stack);
}

void netdiscovery_print_queue_report(QueueHandle_t claw_q, QueueHandle_t netdisc_q) {
    ESP_LOGI(TAG, "Queue Monitoring Report:");
    if (claw_q) {
        uint32_t depth = (uint32_t)uxQueueMessagesWaiting(claw_q);
        uint32_t spaces = (uint32_t)uxQueueSpacesAvailable(claw_q);
        if (depth > s_max_claw_q_depth) s_max_claw_q_depth = depth;
        ESP_LOGI(TAG, "  ESP-Claw Queue     : Depth=%u | Max Observed=%u | Remaining=%u", (unsigned)depth, (unsigned)s_max_claw_q_depth, (unsigned)spaces);
    }
    if (netdisc_q) {
        uint32_t depth = (uint32_t)uxQueueMessagesWaiting(netdisc_q);
        uint32_t spaces = (uint32_t)uxQueueSpacesAvailable(netdisc_q);
        if (depth > s_max_netdisc_q_depth) s_max_netdisc_q_depth = depth;
        ESP_LOGI(TAG, "  NetDiscovery Queue : Depth=%u | Max Observed=%u | Remaining=%u", (unsigned)depth, (unsigned)s_max_netdisc_q_depth, (unsigned)spaces);
    }
}

void netdiscovery_log_ownership_event(uint32_t request_id, const char* call_id, const char* event_type) {
    ESP_LOGI(TAG, "[OWNERSHIP][%u][%s] Lifetime Event: %s", (unsigned)request_id, call_id ? call_id : "N/A", event_type ? event_type : "UNKNOWN");
}

void netdiscovery_run_phase15_stress_test(int num_synthetic_calls) {
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "STARTING PHASE 1.5 STRESS TEST (%d synthetic calls)", num_synthetic_calls);
    ESP_LOGI(TAG, "========================================================");

    netdiscovery_memory_snapshot_t mem_before, mem_after;
    netdiscovery_get_memory_snapshot(&mem_before);

    for (int i = 0; i < num_synthetic_calls; i++) {
        char call_id_buf[32];
        snprintf(call_id_buf, sizeof(call_id_buf), "call_stress_%d", i + 1);

        netdiscovery_pipeline_timestamps_t ts;
        memset(&ts, 0, sizeof(ts));
        ts.t0_webrtc_recv = esp_timer_get_time();
        ts.t1_claw_enqueue = ts.t0_webrtc_recv + 1000;
        ts.t2_lua_recv = ts.t1_claw_enqueue + 500;
        ts.t3_ipc_push = ts.t2_lua_recv + 300;

        netdiscovery_intent_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.version = NETDISCOVERY_IPC_VERSION;
        msg.request_id = 9000 + i;
        strlcpy(msg.call_id, call_id_buf, sizeof(msg.call_id));
        strlcpy(msg.action, "set_volume", sizeof(msg.action));
        strlcpy(msg.target, "living_room_tv", sizeof(msg.target));
        strlcpy(msg.parameters_json, "{\"volume\": 20}", sizeof(msg.parameters_json));

        netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "intent created (stress test)");

        if (netdiscovery_intent_queue) {
            netdiscovery_log_ownership_event(msg.request_id, msg.call_id, "intent queued (stress test)");
            xQueueSend(netdiscovery_intent_queue, &msg, 0);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Allow worker to flush synthetic messages

    netdiscovery_get_memory_snapshot(&mem_after);
    netdiscovery_print_memory_report("Phase 1.5 Stress Test Complete", &mem_before, &mem_after);
}
