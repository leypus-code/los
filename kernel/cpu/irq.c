#include "../include/irq.h"
#include "../include/pic.h"

static irq_handler_t irq_routines[16] = { 0 };

void irq_install_handler(int irq, irq_handler_t handler) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}

void irq_handler(struct registers *regs) {
    int irq = regs->int_no - 32;

    if (irq >= 0 && irq < 16 && irq_routines[irq]) {
        irq_routines[irq](regs);
    }

    pic_send_eoi((unsigned char)irq);
}
