# RTOS Laboratory Work #2: Thread Local Storage (TLS)

This project demonstrates the use of **Thread Local Storage (TLS)** in FreeRTOS to create thread-safe tasks without using global variables. Each task maintains its own unique state and configuration using TLS pointers.

## Description

The goal of this laboratory work is to:
1.  Learn how to use TLS slots (`vTaskSetThreadLocalStoragePointer`, `xTaskGetThreadLocalStoragePointer`)[cite: 3].
2.  Implement thread-safe modules where each task has its own context and execution profile[cite: 4].
3.  Simulate CPU load using "Busy loops" instead of simple delays.
4.  Implement a "Burst" logging mechanism to reduce console I/O overhead[cite: 35].

## Variant Details (Variant 7)

* **Student Variant:** 7 [cite: 44]
* **Total Tasks:** 4 [cite: 44]
* **Execution Mode:** `Busy` (CPU Intensive) [cite: 44]
* **Burst Size:** 7 (Logs are printed in batches of 7 lines) [cite: 44]

### Task Configuration

According to Variant 7, the TLS indices are swapped ($1/0$) compared to the default configuration[cite: 44].

| Parameter | Value | Description |
| :--- | :--- | :--- |
| **TLS Index 0** | `profile_t` | Stores static configuration (mode `BUSY`, cycles). |
| **TLS Index 1** | `task_context_t` | Stores dynamic state (counters, checksums, buffers). |
| **Base Cycles** | 16,000 cycles | Starting CPU load for Task 0[cite: 44]. |
| **Step** | 800 cycles | Increment of load for each subsequent task[cite: 40]. |
| **Iterations** | 47 | Calculated as $N_i = 10 \times N_{tasks} + IDX$ ($10 \times 4 + 7$)[cite: 55]. |

## Implementation Details

### 1. Data Structures (TLS)
Instead of global variables, the following structures are allocated dynamically for each task:
* **Context (V1 - Slot 1):** Contains the task name, current iteration `iter`, CRC8 `checksum`, and a string buffer `line1` for logging.
* **Profile (V2 - Slot 0):** Contains execution settings like `baseCycles` and `burstMode`[cite: 29, 30].

### 2. Logic Flow
1.  **Initialization:** The task allocates memory for Context and Profile, initializes them (Mode = BUSY), and saves pointers to TLS slots (Index 0 for Profile, Index 1 for Context).
2.  **Loop:**
    * Increments iteration counter.
    * Updates CRC8 Checksum based on iteration and a unique seed[cite: 57].
    * Writes a log message to the internal buffer.
    * **Burst Check:** If the buffer has 7 lines, it flushes (prints) them to the console[cite: 59].
    * **Busy Loop:** Instead of sleeping, the task executes a dummy loop for `16000 + (800 * TaskID)` cycles to simulate processing work[cite: 65].
3.  **Cleanup:** Frees allocated memory and deletes the task upon completion[cite: 71].

## Results

The application successfully creates 4 tasks. Since the execution mode is `Busy`, tasks actively consume CPU cycles rather than sleeping.
* **Thread Safety:** Each task calculates its own unique checksum and maintains its own iteration counter correctly, proving that TLS isolates data effectively.
* **Burst Mode:** Logs appear in groups, confirming the buffering mechanism works as expected.

