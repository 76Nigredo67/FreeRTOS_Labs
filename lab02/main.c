/*
 * FreeRTOS Lab #2 - Variant 7 
 */

 /* --- STANDARD INCLUDES --- */
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <stdint.h> 

/* Visual studio intrinsics */
#include <intrin.h>

#ifdef WIN32_LEAN_AND_MEAN
#include "winsock2.h"
#else
#include <winsock.h>
#endif

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "trcRecorder.h"

/* --- LAB 2 CONSTANTS (VARIANT 7) --- */
#define IDX             7           // 
#define N_TASKS         4           // Busy 4
#define BURST_TRIGGER   7           // Burst 7 

/* TLS Indicies swapped for Variant 7 (V1/V2 = 1/0)  */
#define TLS_CTX_INDEX   1           
#define TLS_PROF_INDEX  0           

/* Busy Mode Constants */
#define BASE_CYCLES     16000      
#define STEP_CYCLES     800         
#define BURST_N         10          // Max buffer size safe margin

/* --- HEAP CONFIGURATION --- */
#define mainREGION_1_SIZE     82010
#define mainREGION_2_SIZE     239050
#define mainREGION_3_SIZE     168070

/* --- FUNCTION PROTOTYPES --- */
void vAssertCalled(unsigned long ulLine, const char* const pcFileName);
static void prvInitialiseHeap(void);

/* Hooks prototypes */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName);
void vApplicationTickHook(void);
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, configSTACK_DEPTH_TYPE* puxIdleTaskStackSize);
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize);
void vApplicationDaemonTaskStartupHook(void);

/* --- DATA STRUCTURES --- */
typedef enum run_mode_e {
    MODE_DELAY = 0,
    MODE_BUSY = 1
} run_mode_t;

/* Profile structure (V2) - TLS Slot 0 in Var 7 */
typedef struct profile_s {
    run_mode_t mode;
    uint8_t burstMode;
    uint32_t baseDelayMs;
    uint32_t baseCycles;
    uint8_t step;
} profile_t;

/* Task Context structure (V1) - TLS Slot 1 in Var 7 */
typedef struct task_context_s {
    char name[12];
    uint32_t iter;
    uint32_t checksum;
    uint32_t delayTicksOrCycles;
    uint32_t seed;
    char line1[BURST_N][64];
    uint8_t bcnt;
} task_context_t;

/* --- HELPER FUNCTIONS --- */

/* Function to simulate busy work (processor cycles) */
void busy_loop(uint32_t cycles) {
    volatile uint32_t i;
    for (i = 0; i < cycles; i++) {
        // Prevent compiler optimization removing the loop
        __nop(); 
    }
}

uint8_t crc8_sae_j11850(uint8_t* data, size_t len) {
    uint8_t crc = 0xFF;
    size_t i;
    uint8_t j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x1D;
            }
            else {
                crc <<= 1;
            }
        }
    }
    return crc ^ 0xFF;
}

void burst_flush(task_context_t* ctx) {
    int i;
    if (ctx->bcnt == 0) return;

    taskENTER_CRITICAL();
    for (i = 0; i < ctx->bcnt; i++) {
        printf("%s\n", ctx->line1[i]);
    }
    taskEXIT_CRITICAL();

    ctx->bcnt = 0;
}

/* --- WORKER TASK --- */
void vWorker(void* arg) {
    int task_id = (int)(uintptr_t)arg;
    int Ni, i;
    uint8_t data_to_crc[8];
    uint32_t cycle_load;

    /* Allocate structures in Heap */
    task_context_t* ctx = (task_context_t*)pvPortMalloc(sizeof(task_context_t));
    profile_t* prof = (profile_t*)pvPortMalloc(sizeof(profile_t));

    if (ctx == NULL || prof == NULL) {
        printf("Malloc failed for Task %d\n", task_id);
        if (ctx) vPortFree(ctx);
        if (prof) vPortFree(prof);
        vTaskDelete(NULL);
        return;
    }

    /* Initialize Context */
    snprintf(ctx->name, sizeof(ctx->name), "T02_%d", task_id);
    ctx->iter = 0;
    ctx->checksum = 0;
    ctx->seed = 0xAA + task_id;
    ctx->bcnt = 0;

    /* Initialize Profile for Variant 7 (BUSY MODE) */
    prof->mode = MODE_BUSY;             // 
    prof->burstMode = BURST_TRIGGER;
    prof->baseCycles = BASE_CYCLES;     // 16000
    prof->step = STEP_CYCLES;           // 800

    /* Set TLS Pointers: Note indices are swapped for Var 7 */
    vTaskSetThreadLocalStoragePointer(NULL, TLS_CTX_INDEX, ctx);   // Slot 1
    vTaskSetThreadLocalStoragePointer(NULL, TLS_PROF_INDEX, prof); // Slot 0

    /* Calculate Iterations: Ni = 10 * Ntask + IDX */
    Ni = 10 * N_TASKS + IDX; // 10 * 4 + 7 = 47 iterations

    for (i = 0; i < Ni; ++i) {
        ctx->iter++;

        /* Calculate Checksum */
        memcpy(&data_to_crc[0], &ctx->iter, 4);
        memcpy(&data_to_crc[4], &ctx->seed, 4);
        ctx->checksum = crc8_sae_j11850(data_to_crc, 8);

        /* Log formatting */
        snprintf(ctx->line1[ctx->bcnt], 64,
            "[Task %s] Tick: %lu Iter: %lu Sum: 0x%08X",
            ctx->name, xTaskGetTickCount(), ctx->iter, ctx->checksum);

        ctx->bcnt++;

        /* Burst Check */
        if (ctx->bcnt >= prof->burstMode) {
            burst_flush(ctx);
        }

        /* Perform Work (Busy Loop) instead of Delay */
        // Formula: baseCycles + 800 * task_id 
        cycle_load = prof->baseCycles + (prof->step * task_id);
        busy_loop(cycle_load);
        
        // Optional: Add a minimal yield to allow other tasks to run if cycles are too fast
        // taskYIELD(); 
    }

    /* Flush remaining logs */
    burst_flush(ctx);

    taskENTER_CRITICAL();
    printf("Task finished %d\n", task_id);
    taskEXIT_CRITICAL();

    /* Free Memory and Delete Task */
    vPortFree(ctx);
    vPortFree(prof);
    vTaskDelete(NULL);
}

/* --- MAIN FUNCTION --- */
int main(void)
{
    int i;
    prvInitialiseHeap();
    vTraceEnable(TRC_START);

    printf("\nStarting Lab 2 (Variant 7 - Busy Mode)...\r\n");
    printf("IDX: %d, Tasks: %d, Burst: %d, BaseCycles: %d\n", 
           IDX, N_TASKS, BURST_TRIGGER, BASE_CYCLES);

    for (i = 0; i < N_TASKS; ++i) {
        xTaskCreate(vWorker, "Task", 1024, (void*)(uintptr_t)i, 1, NULL);
    }

    vTaskStartScheduler();
    return 0;
}

/* --- HOOKS (UNCHANGED) --- */
void vApplicationMallocFailedHook(void) { vAssertCalled(__LINE__, __FILE__); }
void vApplicationIdleHook(void) {}
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName) { (void)pxTask; (void)pcTaskName; vAssertCalled(__LINE__, __FILE__); }
void vApplicationTickHook(void) {}
void vApplicationDaemonTaskStartupHook(void) {}

static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, configSTACK_DEPTH_TYPE* puxIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *puxIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize) {
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vAssertCalled(unsigned long ulLine, const char* const pcFileName) {
    printf("ASSERT! Line %ld, file %s\r\n", ulLine, pcFileName);
    __debugbreak();
    for (;; );
}

static void prvInitialiseHeap(void) {
    static uint8_t ucHeap[configTOTAL_HEAP_SIZE];
    const HeapRegion_t xHeapRegions[] = {
        { ucHeap + 1, mainREGION_1_SIZE },
        { ucHeap + 15 + mainREGION_1_SIZE, mainREGION_2_SIZE },
        { ucHeap + 19 + mainREGION_1_SIZE + mainREGION_2_SIZE, mainREGION_3_SIZE },
        { NULL, 0 }
    };
    vPortDefineHeapRegions(xHeapRegions);
}

/* ============================================================ */
/* TRACE RECORDER SUPPORT FUNCTIONS                             */
/* ============================================================ */
static uint32_t ulEntryTime = 0;

void vTraceTimerReset(void)
{
    ulEntryTime = xTaskGetTickCount();
}

uint32_t uiTraceTimerGetFrequency(void)
{
    return configTICK_RATE_HZ;
}

uint32_t uiTraceTimerGetValue(void)
{
    return(xTaskGetTickCount() - ulEntryTime);
}