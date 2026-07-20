#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define INTENT_MAX_LEN 1024
#define CALL_ID_MAX_LEN 64

typedef struct {
    char call_id[CALL_ID_MAX_LEN];
    char intent_json[INTENT_MAX_LEN];
} netdiscovery_intent_t;

extern QueueHandle_t netdiscovery_intent_queue;

// Initializes the NetDiscovery IPC queues and listener tasks
void netdiscovery_ipc_init(void);

// Triggers the initial one-shot SSDP discovery
void netdiscovery_trigger_initial_scan(void);

#ifdef __cplusplus
}
#endif
