*This project has been created as part of the 42 curriculum by dbobrov.*

# Codexion

## Description
Codexion is a concurrency simulation project written in C, simulating coders competing for shared resources (USB dongles) before a burnout deadline. It explores POSIX threads, mutexes, condition variables, and custom scheduling algorithms (FIFO and Earliest Deadline First). The goal is to successfully orchestrate a shared environment without deadlocks, data races, or coder starvation.

## Instructions

### Compilation
A `Makefile` is provided to compile the project seamlessly. Run the following command at the root of the project:
```bash
make
```
This will compile the source files using `cc` with `-Wall -Wextra -Werror -pthread` flags and output the executable `codexion`.

### Execution
The program requires exactly 8 mandatory arguments:
```bash
./codexion <number_of_coders> <time_to_burnout> <time_to_compile> <time_to_debug> <time_to_refactor> <number_of_compiles_required> <dongle_cooldown> <scheduler>
```
* **scheduler**: must be either `fifo` or `edf`.

Example:
```bash
./codexion 5 800 200 200 200 7 50 fifo
```

### Cleanup
To remove object files:
```bash
make clean
```
To remove object files and the executable:
```bash
make fclean
```

## Resources
* [POSIX Threads Programming (Lawrence Livermore National Laboratory)](https://hpc-tutorials.llnl.gov/posix/)
* [Operating Systems: Three Easy Pieces (Concurrency)](https://pages.cs.wisc.edu/~remzi/OSTEP/)

**AI Usage:**
AI was used primarily for code review, debugging race conditions, and refactoring the locking mechanisms to ensure a localized (per-dongle) mutex architecture instead of a global lock, as well as providing feedback on implementing a robust priority queue.

## Blocking cases handled
* **Deadlock Prevention & Coffman's Conditions:** Deadlocks are prevented by breaking the circular wait condition. When acquiring two dongles, a strict lock ordering is enforced: the dongle with the lower index is always requested and locked before the higher index.
* **Starvation Prevention:** A precise priority heap is used to manage dongle requests, processing requests based on either their arrival time (FIFO) or the closest burnout deadline (EDF).
* **Cooldown Handling:** Each dongle tracks its `last_release_ms`. Before assigning a dongle, the simulation checks if the cooldown period has expired; if not, it conditionally waits using `pthread_cond_timedwait`.
* **Precise Burnout Detection:** A dedicated monitor thread polls the state of the coders, checking their local deadlines. It skips coders who have fulfilled their compile quota. Burnout outputs are strictly accurate within 10ms.
* **Log Serialization:** All console output (`printf`) is protected by a dedicated `log_mutex` to guarantee logs never interleave or mix up.

## Thread synchronization mechanisms
* **`pthread_mutex_t` & `pthread_cond_t`:** These primitives form the core of the synchronization. 
  * Each `t_dongle` contains its own mutex and condition variable. This localized approach allows coders to wait on specific resources rather than blocking the entire simulation state.
  * A per-coder `mutex` protects local state like `last_compile_start` and `compile_count` from race conditions between the coder thread and the monitor thread.
  * A global `simulation_mutex` safely tracks the running boolean flag of the simulation.
* **Race Condition Prevention:** By minimizing the scope of each lock (locking exactly before reading/writing shared fields and unlocking immediately after), we avoid race conditions while maintaining high concurrency.
* **Thread-safe Communication:** The `monitor_thread` reads from coders safely by grabbing their individual locks. When a stop condition is met (burnout or quota reached), it sets `running = false` under the `simulation_mutex` and wakes all waiting coders using a `pthread_cond_broadcast` across all dongles, ensuring no threads are left permanently blocked.
