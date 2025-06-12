#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define NCHILDREN 10
#define MAX_MSG_LENGTH 50
#define MAX_WRITES 3
#define SHARED_PAGE_SIZE PGSIZE

static char *shared_buffer = 0;

// Helper: construct "[child X] Hello!\n"
void make_child_message(char *buf, int index) {
    const char *prefix = "[child ";
    const char *suffix = "] Hello!\n";
    int i = 0;

    for (int j = 0; prefix[j]; j++) buf[i++] = prefix[j];

    // Convert number to string
    if (index == 0) buf[i++] = '0';
    else {
        char digits[10];
        int d = 0;
        while (index > 0) {
            digits[d++] = '0' + (index % 10);
            index /= 10;
        }
        for (int j = d - 1; j >= 0; j--) buf[i++] = digits[j];
    }

    for (int j = 0; suffix[j]; j++) buf[i++] = suffix[j];
    buf[i] = '\0';
}

void write_log_entries(int child_index, uint64 shared_va, const char *message) {
    int msg_len = strlen(message);
    uint64 addr = shared_va;
    int writes = 0;

    while (writes < MAX_WRITES && addr + 4 + msg_len < shared_va + SHARED_PAGE_SIZE) {
        uint32 header = (child_index << 16) | (msg_len & 0xFFFF);
        if (__sync_val_compare_and_swap((uint32 *)addr, 0, header) == 0) {
            memcpy((void *)(addr + 4), message, msg_len);
            writes++;
        }
        addr += 4 + msg_len;
        addr = (addr + 3) & ~3; // 4-byte alignment
    }
    exit(0);
}

void read_log_entries(uint64 shared_va) {
    uint64 addr = shared_va;
    int empty_reads = 0;

    while (addr + 4 < shared_va + SHARED_PAGE_SIZE) {
        uint32 header = *(volatile uint32 *)addr;

        if (header == 0) {
            if (++empty_reads > 5) break;
            sleep(1);
            continue;
        }

        empty_reads = 0;

        uint16 child_index = header >> 16;
        uint16 msg_len = header & 0xFFFF;

        printf("[parent] Received message from child %d (len %d) at 0x%lx\n",
               child_index, msg_len, addr);

        char msg[MAX_MSG_LENGTH + 1] = {0};
        memcpy(msg, (void *)(addr + 4), msg_len);
        msg[msg_len] = '\0';

        printf("[parent %d] %s\n", child_index, msg);

        addr += 4 + msg_len;
        addr = (addr + 3) & ~3;
    }

    for (int i = 0; i < NCHILDREN; i++)
        wait(0);
}

int main() {
    int parent_pid = getpid();

    shared_buffer = (char *)malloc(SHARED_PAGE_SIZE);
    if (!shared_buffer) {
        printf("log_test: failed to allocate shared buffer\n");
        exit(1);
    }

    for (int i = 0; i < NCHILDREN; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("log_test: fork failed for child %d\n", i);
            exit(1);
        }

        if (pid == 0) {
            int child_index = i;
            uint64 mapped_va = map_shared_pages(parent_pid, getpid(), (uint64)shared_buffer, SHARED_PAGE_SIZE);

            if (mapped_va == 0 || mapped_va == (uint64)-1) {
                printf("child %d: failed to map shared memory\n", child_index);
                exit(1);
            }

            char message[MAX_MSG_LENGTH];
            make_child_message(message, child_index);
            write_log_entries(child_index, mapped_va, message);
        }
    }

    // Parent process reads log
    read_log_entries((uint64)shared_buffer);
    exit(0);
}
