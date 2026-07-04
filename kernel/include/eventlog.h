#ifndef LOS_EVENTLOG_H
#define LOS_EVENTLOG_H

void eventlog_initialize(void);
void eventlog_add(const char *message);
void eventlog_print(void);

#endif
