#include "../include/keyboard.h"
#include "../include/irq.h"
#include "../include/io.h"
#include "../include/shell.h"
#include "../include/norton.h"
#include "../include/editor.h"
#include "../include/wm.h"
#include "../include/layout.h"
#include "../include/keycodes.h"

static int extended_scancode = 0;
static int shift_down = 0;
static int caps_lock = 0;
static uint32_t shift_presses = 0;

static const char normal[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static const char shifted[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
};

static char apply_case(uint8_t scancode) {
    char c = shift_down ? shifted[scancode] : normal[scancode];

    if (!shift_down && caps_lock && c >= 'a' && c <= 'z') {
        c -= 32;
    }

    if (shift_down && caps_lock && c >= 'A' && c <= 'Z') {
        c += 32;
    }

    return c;
}

static void dispatch_key(int key) {
    int mode = shell_get_ui_mode();

    if (mode == 1) {
        norton_handle_key(key);
        return;
    }

    if (mode == 2) {
        editor_handle_key(key);
        return;
    }

    if (mode == 4) {
        wm_handle_key(key);
        return;
    }

    if (mode == 5) {
        layout_handle_key(key);
        return;
    }

    shell_handle_key(key);
}

static void keyboard_callback(struct registers *regs) {
    (void)regs;
    uint8_t scancode = inb(0x60);

    if (scancode == 0x3A) { caps_lock = !caps_lock; return; }
    if (scancode == 0x2A || scancode == 0x36) { shift_down = 1; shift_presses++; return; }
    if (scancode == 0xAA || scancode == 0xB6) { shift_down = 0; return; }
    if (scancode == 0xE0) { extended_scancode = 1; return; }
    if (scancode & 0x80) { extended_scancode = 0; return; }

    if (extended_scancode) {
        int key = 0;
        if (scancode == 0x48) key = KEY_ARROW_UP;
        else if (scancode == 0x50) key = KEY_ARROW_DOWN;
        else if (scancode == 0x4B) key = KEY_ARROW_LEFT;
        else if (scancode == 0x4D) key = KEY_ARROW_RIGHT;
        else if (scancode == 0x49) key = KEY_PAGE_UP;
        else if (scancode == 0x51) key = KEY_PAGE_DOWN;
        else if (scancode == 0x47) key = KEY_HOME;
        else if (scancode == 0x4F) key = KEY_END;

        extended_scancode = 0;
        if (key) dispatch_key(key);
        return;
    }

    int fk = 0;
    if (scancode == 0x3B) fk = KEY_F1;
    else if (scancode == 0x3C) fk = KEY_F2;
    else if (scancode == 0x3D) fk = KEY_F3;
    else if (scancode == 0x3E) fk = KEY_F4;
    else if (scancode == 0x3F) fk = KEY_F5;
    else if (scancode == 0x40) fk = KEY_F6;
    else if (scancode == 0x41) fk = KEY_F7;
    else if (scancode == 0x42) fk = KEY_F8;
    else if (scancode == 0x43) fk = KEY_F9;
    else if (scancode == 0x44) fk = KEY_F10;

    if (fk) { dispatch_key(fk); return; }

    char c = apply_case(scancode);
    if (c) dispatch_key((int)c);
}

void keyboard_initialize(void) {
    irq_install_handler(1, keyboard_callback);
}

int keyboard_is_capslock_on(void) { return caps_lock; }
int keyboard_is_shift_down(void) { return shift_down; }
uint32_t keyboard_get_shift_presses(void) { return shift_presses; }
