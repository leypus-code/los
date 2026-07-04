#ifndef LOS_RING_H
#define LOS_RING_H

void ring_initialize(void);
void ring_set_state(const char *state);
void ring_status(void);
int ring_handle_command(const char *command);
int ring_chat(const char *text);

#endif
