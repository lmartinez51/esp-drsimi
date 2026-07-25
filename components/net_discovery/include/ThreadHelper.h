/**
 * @file ThreadHelper.h
 * @brief Provides a thread-safe wrapper for creating ESP-IDF pthreads.
 */

#pragma once

#include <thread>
#include <mutex>
#include <utility>
#include "esp_pthread.h"
#include "esp_heap_caps.h"

namespace NetDiscovery {

class ThreadHelper {
public:
    /**
     * @brief Starts a new std::thread with specific ESP-IDF pthread configuration.
     * 
     * Uses a global mutex to prevent race conditions when configuring the pthread
     * attributes since esp_pthread_set_cfg affects the NEXT thread created.
     */
    template<typename Function>
    static bool StartPinnedThread(const char* name, int stackSize, int priority, int coreId, Function f) {
        // Dynamically allocate the lambda/functor so it survives the task creation
        auto* funcPtr = new Function(std::move(f));

        // Capture-less trampoline decays to a C function pointer compatible with FreeRTOS
        auto trampoline = [](void* arg) {
            auto* pFunc = static_cast<Function*>(arg);
            (*pFunc)(); // Execute the actual lambda
            delete pFunc;
            vTaskDelete(NULL);
        };

        if (xTaskCreatePinnedToCoreWithCaps(trampoline, name, stackSize, funcPtr, priority, NULL, coreId, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) {
            delete funcPtr;
            printf("ThreadHelper: Failed to create task %s in PSRAM\n", name);
            return false;
        }
        return true;
    }

    template<typename Function>
    static bool StartInternalPinnedThread(const char* name, int stackSize, int priority, int coreId, Function f) {
        auto* funcPtr = new Function(std::move(f));

        auto trampoline = [](void* arg) {
            auto* pFunc = static_cast<Function*>(arg);
            (*pFunc)();
            delete pFunc;
            vTaskDelete(NULL);
        };

        if (xTaskCreatePinnedToCore(trampoline, name, stackSize, funcPtr, priority, NULL, coreId) != pdPASS) {
            delete funcPtr;
            printf("ThreadHelper: Failed to create task %s in Internal RAM\n", name);
            return false;
        }
        return true;
    }

private:
    static std::mutex& GetMutex() {
        static std::mutex s_mutex;
        return s_mutex;
    }
};

} // namespace NetDiscovery
