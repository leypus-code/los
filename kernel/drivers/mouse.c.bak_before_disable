#include "../include/mouse.h"
#include "../include/io.h"
#include "../include/irq.h"
#include "../include/gfx.h"
#include <stdint.h>

static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];
static uint32_t mouse_debug_count = 0;

static int wait_input_empty(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x02) == 0) return 1;
    }
    return 0;
}

static int wait_output_full(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if (inb(0x64) & 0x01) return 1;
    }
    return 0;
}

static void ps2_flush(void) {
    for (int i = 0; i < 64; i++) {
        if (inb(0x64) & 0x01) {
            (void)inb(0x60);
        } else {
            return;
        }
    }
}

static void ps2_cmd(uint8_t cmd) {
    wait_input_empty();
    outb(0x64, cmd);
}

static void ps2_data(uint8_t data) {
    wait_input_empty();
    outb(0x60, data);
}

static uint8_t ps2_read(void) {
    if (!wait_output_full()) return 0;
    return inb(0x60);
}

static void mouse_write(uint8_t data) {
    wait_input_empty();
    outb(0x64, 0xD4);
    wait_input_empty();
    outb(0x60, data);
}

static void mouse_debug_flash(uint8_t b) {
    /*
     * Shows whether mouse bytes are received at all.
     * Blue bar length changes on every mouse byte.
     */
    mouse_debug_count++;

    if (!gfx_is_ready()) return;

    uint32_t w = 8 + ((mouse_debug_count % 20) * 8);
    gfx_fill_rect(60, 70, 180, 10, 0x00070910);
    gfx_fill_rect(60, 70, w, 10, 0x005ab4ff);

    if (b & 1) {
        gfx_fill_rect(250, 68, 12, 12, 0x00ffffff);
    } else {
        gfx_fill_rect(250, 68, 12, 12, 0x00070910);
    }
}

static void mouse_process_byte(uint8_t data) {
    mouse_debug_flash(data);

    /*
     * First byte must have bit 3 set.
     */
    if (mouse_cycle == 0 && !(data & 0x08)) {
        return;
    }

    mouse_packet[mouse_cycle++] = data;

    if (mouse_cycle < 3) {
        return;
    }

    mouse_cycle = 0;

    uint8_t flags = mouse_packet[0];
    int dx = (int)(int8_t)mouse_packet[1];
    int dy = (int)(int8_t)mouse_packet[2];

    /*
     * Ignore overflow packets.
     */
    if (flags & 0x40) dx = 0;
    if (flags & 0x80) dy = 0;

    gfx_mouse_move(dx, dy, flags & 0x07);
}

static void mouse_callback(struct registers *regs) {
    (void)regs;

    uint8_t status = inb(0x64);

    if (!(status & 0x01)) return;
    if (!(status & 0x20)) return;

    mouse_process_byte(inb(0x60));
}

void mouse_poll(void) {
    /*
     * Pure AUX polling fallback.
     */
    for (int i = 0; i < 16; i++) {
        uint8_t status = inb(0x64);

        if (!(status & 0x01)) {
            return;
        }

        /*
         * If output is not AUX/mouse data, do not consume it.
         */
        if (!(status & 0x20)) {
            return;
        }

        mouse_process_byte(inb(0x60));
    }
}

void mouse_initialize(void) {
    uint8_t config;

    mouse_cycle = 0;
    ps2_flush();

    /*
     * Enable AUX mouse port.
     */
    ps2_cmd(0xA8);
    ps2_flush();

    /*
     * Read controller config byte.
     */
    ps2_cmd(0x20);
    config = ps2_read();

    /*
     * Enable IRQ12, enable AUX clock, keep translation state untouched.
     */
    config |= 0x02;
    config &= (uint8_t)~0x20;

    /*
     * Write controller config byte.
     */
    ps2_cmd(0x60);
    ps2_data(config);

    ps2_flush();

    /*
     * Reset defaults.
     */
    mouse_write(0xF6);
    (void)ps2_read();

    /*
     * Set sample rate 100.
     */
    mouse_write(0xF3);
    (void)ps2_read();
    mouse_write(100);
    (void)ps2_read();

    /*
     * Enable streaming.
     */
    mouse_write(0xF4);
    (void)ps2_read();

    irq_install_handler(12, mouse_callback);
}
