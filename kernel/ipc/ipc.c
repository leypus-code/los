#include "../include/ipc.h"
#include "../include/eventlog.h"

static ipc_message_t queue[IPC_QUEUE_SIZE];
static uint32_t queue_count = 0;

static void copy_payload(char *dst, const char *src) {
    uint32_t i = 0;

    while (src && src[i] && i < IPC_PAYLOAD_SIZE - 1) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

void ipc_initialize(void) {
    queue_count = 0;
    eventlog_add("ipc initialized");
}

int ipc_send(uint32_t from_pid, uint32_t to_pid, uint32_t type, const char *payload) {
    if (queue_count >= IPC_QUEUE_SIZE) {
        return 0;
    }

    queue[queue_count].from_pid = from_pid;
    queue[queue_count].to_pid = to_pid;
    queue[queue_count].type = type;
    copy_payload(queue[queue_count].payload, payload);

    queue_count++;
    eventlog_add("ipc message sent");
    return 1;
}

int ipc_receive(uint32_t to_pid, ipc_message_t *out) {
    for (uint32_t i = 0; i < queue_count; i++) {
        if (queue[i].to_pid == to_pid) {
            if (out) {
                out->from_pid = queue[i].from_pid;
                out->to_pid = queue[i].to_pid;
                out->type = queue[i].type;

                for (uint32_t k = 0; k < IPC_PAYLOAD_SIZE; k++) {
                    out->payload[k] = queue[i].payload[k];
                }
            }

            for (uint32_t j = i + 1; j < queue_count; j++) {
                queue[j - 1].from_pid = queue[j].from_pid;
                queue[j - 1].to_pid = queue[j].to_pid;
                queue[j - 1].type = queue[j].type;

                for (uint32_t k = 0; k < IPC_PAYLOAD_SIZE; k++) {
                    queue[j - 1].payload[k] = queue[j].payload[k];
                }
            }

            queue_count--;
            eventlog_add("ipc message received");
            return 1;
        }
    }

    return 0;
}

uint32_t ipc_pending_count(uint32_t to_pid) {
    uint32_t count = 0;

    for (uint32_t i = 0; i < queue_count; i++) {
        if (queue[i].to_pid == to_pid) {
            count++;
        }
    }

    return count;
}

#include "../include/kprintf.h"

void ipc_print_inbox(uint32_t to_pid) {
    uint32_t count = 0;

    for (uint32_t i = 0; i < queue_count; i++) {
        if (queue[i].to_pid == to_pid) {
            kprintf("[%u] from %u type %u: %s\n",
                count,
                queue[i].from_pid,
                queue[i].type,
                queue[i].payload
            );
            count++;
        }
    }

    if (count == 0) {
        kprintf("Inbox empty for PID %u\n", to_pid);
    }
}
