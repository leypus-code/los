#ifndef LOS_SERVICE_H
#define LOS_SERVICE_H

void service_initialize(void);
void service_list(void);
int service_call(const char *service, const char *action, const char *arg);

#endif
