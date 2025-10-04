# Operating Systems (2025 Assignment 3): Memory Management and Multi-process Logging in xv6

This repository contains the solution for Assignment 3 of the Operating Systems course, focusing on extending the **xv6** operating system kernel to support **shared memory** and implementing a **multi-process logging system** that utilizes atomic operations.

The assignment was completed in Spring 2025.

## Overview

This assignment is broken down into two main tasks:

1.  **Task 1: Memory Sharing:** Implementing kernel-level support for inter-process shared memory in xv6.
2.  **Task 2: Multi-process Logging:** Building a simple, concurrent logging mechanism that uses the shared memory system from Task 1, employing atomic operations for thread safety.

---

## Task 1: Memory Sharing Implementation

This task involved modifying the xv6 kernel to allow a memory segment from one process's address space to be mapped into another process's address space.

### Key Kernel Functions Implemented/Modified

The following kernel functions were implemented or modified to support memory sharing:

* [cite_start]`uint64 map_shared_pages (struct proc* src_proc, struct proc* dst_proc, uint64 src_va, uint64 size)`: Maps physical pages corresponding to a virtual address (`src_va`) in the source process (`src_proc`) into the destination process (`dst_proc`)[cite: 65, 66, 69].
    * [cite_start]This function handles page alignment and size calculation, maintains the destination process's address space size (`sz` field), and sets a new `PTE_S` flag[cite: 72, 73, 78, 81].
* [cite_start]`uint64 unmap_shared_pages (struct proc* p, uint64 addr, uint64 size)`: Unmaps the shared memory from the destination process, updating the process size (`sz`) and checking that the mapping exists and is a shared mapping[cite: 91, 93, 94].
* [cite_start]**Modification to `uvmunmap()`**: The existing `uvmunmap()` function was modified to prevent freeing physical pages that are marked as shared (`PTE_S`), ensuring that pages are only freed if they were originally allocated by the process being deleted[cite: 95, 100].
* [cite_start]**System Calls**: The following system calls were exposed to userspace to allow applications to use the new memory sharing functionality[cite: 104, 105]:
    * `sys_map_shared_pages()`
    * `sys_unmap_shared_pages()`

### Verification

The shared memory implementation was verified using the user-level test program `shmem_test.c`, which demonstrated:

* [cite_start]Creating a shared mapping from a parent to a child process[cite: 108].
* [cite_start]Interprocess communication (child writes, parent reads)[cite: 109].
* [cite_start]Correct unmapping and memory allocation in the child process[cite: 112, 114].
* [cite_start]Proper cleanup upon process exit, ensuring shared pages are *not* freed by the exiting process[cite: 117, 118].

---

## Task 2: Multi-process Logging

This task implemented a simple, concurrent logging system using the shared memory buffer created in Task 1.

### Logging Protocol and Structure

[cite_start]The logging system enables multiple child processes to write messages to a shared memory buffer concurrently[cite: 132, 134].

* [cite_start]**Header**: Each log message begins with a 32-bit header encoding two `uint16` values[cite: 139, 140]:
    * `uint16 child index`: The unique identifier of the process that wrote the message.
    * [cite_start]`uint16 message length`: The length of the message body (excluding the null terminator)[cite: 140, 141].
* [cite_start]**Free Segment**: A zero-value header indicates a free segment of the buffer[cite: 141].
* [cite_start]**Alignment**: All processes advance their address by the message length plus 4 bytes (for the header), and then align to the next 4-byte boundary for the next header[cite: 150, 151].

### Atomic Operations

[cite_start]To manage concurrent writes to the shared buffer, **atomic operations** were used[cite: 137, 144].

* [cite_start]**Claiming a Segment**: A child process claims a free segment by attempting to write its non-zero header[cite: 141]. This is done using the built-in function:
    [cite_start]`__sync_val_compare_and_swap(ptr, oldval, newval)`[cite: 145].
* [cite_start]This operation atomically compares the value at a memory location (`ptr`) with `oldval` (the expected zero-value for a free segment) and, if equal, replaces it with the `newval` (the child's new header)[cite: 146]. [cite_start]This prevents multiple processes from simultaneously claiming the same log segment[cite: 137].

### Verification

The multi-process logging was verified using the user-level test program `log_test.c`, which involves:

* [cite_start]Forking a minimum of 4 child processes[cite: 154, 183].
* [cite_start]Parent process mapping the shared buffer into each child process[cite: 136, 183].
* [cite_start]Child processes concurrently writing messages and including their unique index[cite: 157, 185].
* [cite_start]The parent process scanning the buffer, reading non-zero headers, and printing the message along with the originating child index[cite: 143, 158, 186].
* [cite_start]Proper boundary checks to ensure a process exits if its address exceeds the shared buffer size[cite: 152, 161].

---

## Files Included

| File | Description |
| :--- | :--- |
| `kernel/*` | Modified xv6 kernel files (e.g., `vm.c`, `proc.c`, `syscall.c`, `riscv.h`) containing the shared memory implementation and the `PTE_S` flag. |
| `user/shmem_test.c` | Userspace program to test the functionality of `map_shared_pages` and `unmap_shared_pages`. |
| `user/log_test.c` | Userspace program implementing the multi-process logging system with atomic operations. |
| `Makefile` | Updated to compile the new user programs. |