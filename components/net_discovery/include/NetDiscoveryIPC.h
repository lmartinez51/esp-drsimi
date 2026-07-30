#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NETDISCOVERY_IPC_VERSION 1

#define IPC_CALL_ID_MAX_LEN    64
#define IPC_ACTION_MAX_LEN     64
#define IPC_TARGET_MAX_LEN    128
#define IPC_ENTITY_ID_MAX_LEN  64
#define IPC_ROOM_MAX_LEN       64
#define IPC_PARAMS_MAX_LEN    512

typedef struct {
    uint16_t version;                           // Protocol version (initial version = 1)
    uint32_t request_id;                        // Internal monotonic execution request ID
    char call_id[IPC_CALL_ID_MAX_LEN];          // OpenAI function call ID for response routing
    char action[IPC_ACTION_MAX_LEN];            // Requested action string (e.g. "set_volume")
    char target[IPC_TARGET_MAX_LEN];            // Target device description/alias (e.g. "living_room_tv")
    char entity_id[IPC_ENTITY_ID_MAX_LEN];      // Direct entity UUID (optional, if bound)
    char room[IPC_ROOM_MAX_LEN];                // Target room metadata (optional)
    char parameters_json[IPC_PARAMS_MAX_LEN];   // Parameter JSON string (e.g. {"volume": 20})
} netdiscovery_intent_t;

// Standardized Execution Error Status Enums (Reserved for Phase 2+)
typedef enum {
    NETDISC_IPC_OK = 0,
    NETDISC_IPC_ERR_INVALID_VERSION = 101,
    NETDISC_IPC_ERR_MISSING_REQUIRED_FIELD = 102,
    NETDISC_IPC_ERR_STRING_TOO_LONG = 103,
    NETDISC_IPC_ERR_SEMANTIC_LOOKUP_FAILED = 201,
    NETDISC_IPC_ERR_CAPABILITY_MISSING = 202,
    NETDISC_IPC_ERR_CONTROLLER_NOT_FOUND = 203,
    NETDISC_IPC_ERR_CANCELLED = 301,
    NETDISC_IPC_ERR_SYSTEM_BUSY = 401
} netdiscovery_ipc_status_t;

// Standardized Execution Result Struct (Reserved for Phase 2+)
typedef struct {
    uint32_t request_id;
    char call_id[IPC_CALL_ID_MAX_LEN];
    netdiscovery_ipc_status_t status;
    char error_message[128];
    char result_json[512];
    uint32_t elapsed_ms;
} netdiscovery_execution_result_t;

extern QueueHandle_t netdiscovery_intent_queue;

// Validates intent struct integrity before semantic execution
bool netdiscovery_validate_intent(const netdiscovery_intent_t* msg);

// Initializes the NetDiscovery IPC queues and listener tasks
void netdiscovery_ipc_init(void);

// Creates the statically-allocated nd_store_writer task and its job queue.
// Idempotent; invoked automatically by netdiscovery_ipc_init().
void netdiscovery_init_writer_task(void);

// Enqueues an asynchronous LittleFS write. On success (true) the writer task
// takes ownership of json_buf and frees it after writing; on failure (false)
// ownership stays with the caller.
bool netdiscovery_submit_store_write(const char* path, char* json_buf, size_t len);

// Enqueues an asynchronous LittleFS delete job (executed safely from Internal DRAM stack).
bool netdiscovery_submit_store_delete(const char* path);

// Cancels and purges any queued asynchronous LittleFS write matching path.
void netdiscovery_cancel_store_write(const char* path);

// Triggers the initial one-shot SSDP discovery.
// Returns true if the scan task was created; false on allocation failure
// (caller must NOT arm the scan timeout timer in that case).
bool netdiscovery_trigger_initial_scan(void);

// Strategic hook reserved for future cancellation by request_id
bool netdiscovery_cancel_request(uint32_t request_id);

#ifdef __cplusplus
}
#endif
