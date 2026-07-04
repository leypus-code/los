#ifndef LOS_KERNEL_H
#define LOS_KERNEL_H

void kernel_init(void);
void kernel_run(void);
void kernel_panic(const char *message);

#endif
