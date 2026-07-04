#ifndef LOS_IRQ_H
#define LOS_IRQ_H

#include <stdint.h>
#include "isr.h"

typedef void (*irq_handler_t)(struct registers *regs);

void irq_install_handler(int irq, irq_handler_t handler);
void irq_uninstall_handler(int irq);
void irq_initialize(void);
void irq_handler(struct registers *regs);

#endif
