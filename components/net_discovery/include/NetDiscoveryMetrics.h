#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int64_t t0_webrtc_recv;    // Function call received by WebRTC
    int64_t t1_claw_enqueue;   // Rule posted to ESP-Claw
    int64_t t2_lua_recv;       // Lua receives rule in register_rule
    int64_t t3_ipc_push;       // IPC queue push in c_send_intent
    int64_t t4_ipc_pop;        // IPC queue pop in listener task
    int64_t t5_validator_done; // Intent Validator completed
    int64_t t6_semantic_done;  // SemanticOrchestrator stub completed
    int64_t t7_response_sent;  // send_function_output invoked
} netdiscovery_pipeline_timestamps_t;

typedef struct {
    size_t internal_free;
    size_t internal_largest_block;
    size_t psram_free;
    size_t psram_largest_block;
} netdiscovery_memory_snapshot_t;

// Memory snapshot helper
void netdiscovery_get_memory_snapshot(netdiscovery_memory_snapshot_t* snapshot);
void netdiscovery_print_memory_report(const char* label, const netdiscovery_memory_snapshot_t* before, const netdiscovery_memory_snapshot_t* after);

// Pipeline latency report
void netdiscovery_print_latency_report(uint32_t request_id, const char* call_id, const netdiscovery_pipeline_timestamps_t* ts);

// Task Stack usage high water mark report
void netdiscovery_print_stack_report(TaskHandle_t claw_task, TaskHandle_t ipc_task);

// Queue depth & monitoring
void netdiscovery_print_queue_report(QueueHandle_t claw_q, QueueHandle_t netdisc_q);

// Object ownership & lifetime tracking
void netdiscovery_log_ownership_event(uint32_t request_id, const char* call_id, const char* event_type);

// Phase 1.5 Stress Test Diagnostic Mode
void netdiscovery_run_phase15_stress_test(int num_synthetic_calls);

#ifdef __cplusplus
}
#endif
