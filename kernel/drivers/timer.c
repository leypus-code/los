#include "../include/timer.h"
#include "../include/irq.h"
#include "../include/io.h"

static uint32_t timer_ticks = 0;

static void timer_callback(struct registers *regs) {
    (void)regs;
    timer_ticks++;
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

void timer_initialize(uint32_t frequency) {
    irq_install_handler(0, timer_callback);

    uint32_t divisor = 1193180 / frequency;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}
