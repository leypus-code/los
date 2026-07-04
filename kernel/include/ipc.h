#ifndef LOS_IPC_H
#define LOS_IPC_H

#include <stdint.h>

#define IPC_PAYLOAD_SIZE 64
#define IPC_QUEUE_SIZE 32

typedef struct ipc_message {
    uint32_t from_pid;
    uint32_t to_pid;
    uint32_t type;
    char payload[IPC_PAYLOAD_SIZE];
} ipc_message_t;

void ipc_initialize(void);
int ipc_send(uint32_t from_pid, uint32_t to_pid, uint32_t type, const char *payload);
int ipc_receive(uint32_t to_pid, ipc_message_t *out);
uint32_t ipc_pending_count(uint32_t to_pid);
void ipc_print_inbox(uint32_t to_pid);

#endif
