# Operating Systems – Assignment 3: Memory Management

This repository contains my implementation of **Assignment 3** in the *Operating Systems (OS 202.1.3031, Spring 2025)* course at **Ben-Gurion University of the Negev**.  
The assignment focuses on extending the xv6 operating system to support **shared memory between processes** and implementing a **multi-process logging system** using this mechanism.

---

## Overview

The goal of this project is to understand and implement low-level **memory management** features in xv6.  
This includes:

1. Implementing a **shared memory mechanism** that allows processes to share physical pages safely.  
2. Building a **multi-process logging system** that uses atomic operations and shared memory to synchronize concurrent writes.

These tasks provide a deeper understanding of how **virtual memory**, **page tables**, and **interprocess communication (IPC)** operate in an OS kernel.

---

## Project Structure

```
xv6/
├── kernel/
│   ├── vm.c, proc.c, sysproc.c, syscall.c   # Modified for shared memory mapping
│   ├── defs.h, riscv.h                      # Added definitions and PTE_S flag
│   └── ...                                  # Other core xv6 kernel files
│
├── user/
│   ├── shmem_test.c                         # Test for shared memory between processes
│   ├── log_test.c                           # Multi-process logging system
│   └── user.h                               # Added system call declarations
│
└── Makefile                                 # Updated to compile new user programs
```

---

## Task 1 – Shared Memory

### Description

In this task, xv6 was extended to support **shared memory mapping** between processes.  
The following kernel functions were implemented:

```c
uint64 map_shared_pages(struct proc* src_proc, struct proc* dst_proc, uint64 src_va, uint64 size);
uint64 unmap_shared_pages(struct proc* p, uint64 addr, uint64 size);
```

These functions use `walk()`, `mappages()`, and `uvmunmap()` to map or unmap physical pages from one process to another.

### Key Details

- Each shared mapping sets a new flag:
  ```c
  #define PTE_S (1L << 8)  // shared page
  ```
- Physical pages are freed **only by their owning process** when it exits.  
- The kernel’s `uvmunmap()` and `vm.c` logic were updated to prevent double freeing of shared pages.  
- Two system calls were added for user-level access:
  ```c
  int sys_map_shared_pages(void);
  int sys_unmap_shared_pages(void);
  ```

### Test Program – `shmem_test.c`

The test program verifies shared memory functionality by:  
1. Creating a shared mapping between parent and child.  
2. Writing a message (“Hello daddy”) from the child and printing it in the parent.  
3. Unmapping shared memory and confirming `malloc()` works correctly again.  
4. Printing process memory size before/after mapping and unmapping.  
5. Testing cleanup and persistence when a child exits without unmapping.

---

## Task 2 – Multi-Process Logging

### Description

The second part builds a **multi-process logging system** using shared memory.  
Multiple child processes write log messages to a shared buffer, while the parent reads them.

### Implementation

- Each child writes messages using a 4-byte header:
  ```c
  uint16 child_index;
  uint16 message_length;
  ```
- Atomic operations ensure safe concurrent writes:
  ```c
  __sync_val_compare_and_swap();
  ```
- Alignment is preserved with:
  ```c
  addr = (addr + 3) & ~3;
  ```
- The parent process scans and prints messages along with their originating process index.

### Test Program – `log_test.c`

1. Fork at least 4 child processes.  
2. Map a shared buffer across them.  
3. Each child writes multiple messages to the buffer using atomic operations.  
4. The parent reads and prints all logs in order.  
5. Tested with different message lengths and buffer-overflow conditions.

---

## Learning Outcomes

- Understanding and implementing **virtual memory mapping** in xv6.  
- Building and debugging **shared memory systems** in a real kernel.  
- Using **atomic operations** for safe synchronization between concurrent writers.  
- Designing **user-level IPC systems** using low-level primitives.  
- Handling **alignment**, **page-table flags**, and **memory cleanup** properly.

---

## Build and Run

1. Clean and rebuild xv6:
   ```bash
   make clean
   make qemu
   ```

2. Run shared memory test:
   ```bash
   shmem_test
   ```

3. Run multi-process logging test:
   ```bash
   log_test
   ```

4. Verify that:  
   - Shared memory mappings are created and cleaned correctly.  
   - Multiple processes can safely log messages without corruption or crashes.

---

## References

- xv6: A simple, Unix-like teaching operating system  
- Course Material: Operating Systems (202.1.3031) – Ben-Gurion University, Spring 2025  
- RISC-V Architecture Specification and xv6 source documentation  

---

## Repository Name Suggestion

**xv6-Memory-Management-And-IPC**

This name matches your previous convention (`xv6-Processes-And-Syscalls`, `xv6-Synchronization-And-Processes`) and reflects the focus on memory sharing and interprocess communication.
