Lab 1: Introduction to FreeRTOS

## Description

This lab covers the basics of FreeRTOS by creating, running, and monitoring three periodic tasks. The primary goals were:
1.  To create three periodic tasks ($\t_2, \t_1, \t_3$) based on a calculated variant.
2.  To implement a "busy-wait" loop to simulate computational work ($C_i$).
3.  To log start and finish timestamps (in ticks) for each task.
4.  To observe and confirm priority-based preemption.

## Variant Details (ID = 7)

Based on the student ID `7`, the following parameters were calculated:

* **Prefix:** `SHEM_`
* **Periods:**
    * T1 = 13 ms
    * T2 = 23 ms
    * T3 = 41 ms
* **Compute Times:**
    * C1 = 3 ms
    * C2 = 5 ms
    * C3 = 5 ms
* **Priorities:**
    * Rule: $\t_2 > \t_1 > \t_3$ (since ID is even)
    * `SHEM_Task1`: Priority 2
    * `SHEM_Task2`: Priority 3 (Highest)
    * `SHEM_Task3`: Priority 1 (Lowest)

---

## Results

The program was successfully compiled and executed. The console log shows all three tasks running periodically according to their specified parameters.

The key result is the clear demonstration of **priority-based preemption**. The log shows that the higher-priority task (`SHEM_Task1`) correctly interrupts (preempts) the lower-priority task (`SHEM_Task2`) as soon as it becomes ready.

### Log Snippet: Preemption Example

This snippet shows the scheduler preempting `Task2` in favor of `Task1`.