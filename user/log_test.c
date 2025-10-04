#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define NCHILDREN 10
#define MAX_MSG_LENGTH 50
#define MAX_WRITES 3
#define SHARED_PAGE_SIZE PGSIZE

static char *shared_buffer = 0;

//convert messages into a buffer
void make_child_message(char *buf, int index) {
    const char *prefix = "[child ";
    const char *suffix = "] Hi there\n";
    int i = 0;

    for (int j = 0; prefix[j]; j++){
        buf[i++] = prefix[j];
    } 

    // convert number to string
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

    for (int j = 0; suffix[j]; j++){
        buf[i++] = suffix[j];
        buf[i] = '\0';
    } 
}

//children write messages
void write_log_entries(int child_index, uint64 shared_va, const char *message) {
    int msg_len = strlen(message);
    uint64 addr = shared_va;
    int writes = 0;

     // attempt to write up to MAX_WRITES messages in loop 
    while (writes < MAX_WRITES && addr + 4 + msg_len < shared_va + SHARED_PAGE_SIZE) {
        uint32 header = (child_index << 16) | (msg_len & 0xFFFF);
        // writes message according to the location ( header or after)
        if (__sync_val_compare_and_swap((uint32 *)addr, 0, header) == 0) { //Tries to find a free space (header == 0)
            memcpy((void *)(addr + 4), message, msg_len);
            writes++;
        }
        addr += 4 + msg_len;
        addr = (addr + 3) & ~3; // 4-byte alignment "byte boundary"
    }
    exit(0);
}

void read_log_entries(uint64 shared_va) {
    uint64 addr = shared_va;
    int empty_reads = 0;
    int total_received = 0;
    const int expected = NCHILDREN * MAX_WRITES;

    // loop until all expected messages have been received
    while (total_received < expected) {
        uint32 header = *(volatile uint32 *)addr;

        if (header == 0) { //empty header
            if (++empty_reads > 5) {
                sleep(1); // avoid busy-waiting
                empty_reads = 0;
            }
        } 
        else {
            empty_reads = 0;

            //child index and message length from header
            uint16 child_index = header >> 16;
            uint16 msg_len = header & 0xFFFF;

            if (msg_len > MAX_MSG_LENGTH) {
                printf("Invalid msg_len=%d at addr=0x%lx. Skipping.\n", msg_len, addr);
                addr += 4; 
                continue;
            }

            printf("[parent] Received message from child %d (len %d) at 0x%lx\n",
                   child_index, msg_len, addr);
            
            // read and print the message content
            char msg[MAX_MSG_LENGTH + 1] = {0};
            memcpy(msg, (void *)(addr + 4), msg_len);
            msg[msg_len] = '\0';

            printf("[parent %d] %s\n",child_index, msg);

            total_received++;
            addr += 4 + msg_len;
        }

        addr = (addr + 3) & ~3;
        if (addr + 4 >= shared_va + SHARED_PAGE_SIZE) {
            addr = shared_va;
        }
    }
}

int main() {
    int parent_pid = getpid();

    // allocating a shared page-sized buffer

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
        // child process
        if (pid == 0) {
            int child_index = i;

            // map the shared buffer into the child’s address space
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

    // parent process reads log
    read_log_entries((uint64)shared_buffer);
    exit(0);
}
