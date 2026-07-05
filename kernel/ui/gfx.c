#include "../include/gfx.h"
#include "../include/kernel.h"

static int gfx_ready = 0;
static uint32_t gfx_addr = 0;
static uint32_t gfx_width = 0;
static uint32_t gfx_height = 0;
static uint32_t gfx_pitch = 0;
static uint32_t gfx_bpp = 0;
static uint32_t gfx_anim_tick = 0;
static int gfx_model_state = 0; /* 0 idle, 1 loading, 2 ready, 3 thinking, 4 drawing */
static int gfx_ring_anchor = 0; /* 0 center, 1 workspace top-right */

#define MODEL_RING_BOX 320
static uint32_t gfx_model_ring_buffer[MODEL_RING_BOX * MODEL_RING_BOX];

static void gfx_buf_clear(uint32_t color) {
    for (uint32_t i = 0; i < MODEL_RING_BOX * MODEL_RING_BOX; i++) {
        gfx_model_ring_buffer[i] = color;
    }
}

static void gfx_buf_put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || y < 0) return;
    if (x >= MODEL_RING_BOX || y >= MODEL_RING_BOX) return;

    gfx_model_ring_buffer[(uint32_t)y * MODEL_RING_BOX + (uint32_t)x] = color;
}

static void gfx_buf_fill_circle(int cx, int cy, int r, uint32_t color) {
    if (r <= 0) return;

    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if ((x * x + y * y) <= (r * r)) {
                gfx_buf_put_pixel(cx + x, cy + y, color);
            }
        }
    }
}

static void gfx_buf_draw_ring_circle(int cx, int cy, int r_outer, int thickness, uint32_t color) {
    if (r_outer <= 0 || thickness <= 0) return;

    int r_inner = r_outer - thickness;
    if (r_inner < 0) r_inner = 0;

    int outer2 = r_outer * r_outer;
    int inner2 = r_inner * r_inner;

    for (int y = -r_outer; y <= r_outer; y++) {
        for (int x = -r_outer; x <= r_outer; x++) {
            int d2 = x * x + y * y;

            if (d2 <= outer2 && d2 >= inner2) {
                gfx_buf_put_pixel(cx + x, cy + y, color);
            }
        }
    }
}

static void gfx_blit_model_ring_buffer(int screen_cx, int screen_cy) {
    int start_x = screen_cx - (MODEL_RING_BOX / 2);
    int start_y = screen_cy - (MODEL_RING_BOX / 2);

    for (int y = 0; y < MODEL_RING_BOX; y++) {
        for (int x = 0; x < MODEL_RING_BOX; x++) {
            int px = start_x + x;
            int py = start_y + y;

            if (px >= 0 && py >= 0 && px < (int)gfx_width && py < (int)gfx_height) {
                gfx_put_pixel(
                    (uint32_t)px,
                    (uint32_t)py,
                    gfx_model_ring_buffer[(uint32_t)y * MODEL_RING_BOX + (uint32_t)x]
                );
            }
        }
    }
}


static uint32_t gfx_cursor_tick = 0;
static int gfx_cursor_visible = 1;

static int gfx_surface_mode = 0; /* 0 = AI, 1 = Shell */
static int gfx_mouse_x = 400;
static int gfx_mouse_y = 300;
static uint8_t gfx_mouse_buttons = 0;

static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void gfx_initialize_from_kernel(void) {
    uint32_t fb_addr = kernel_framebuffer_addr_low();
    uint32_t fb_addr_high = kernel_framebuffer_addr_high();

    gfx_ready = 0;
    gfx_addr = 0;
    gfx_width = 0;
    gfx_height = 0;
    gfx_pitch = 0;
    gfx_bpp = 0;

    if (!kernel_framebuffer_available()) {
        return;
    }

    if (fb_addr_high != 0) {
        return;
    }

    gfx_bpp = kernel_framebuffer_bpp();

    if (gfx_bpp != 32) {
        return;
    }

    gfx_addr = fb_addr;
    gfx_width = kernel_framebuffer_width();
    gfx_height = kernel_framebuffer_height();
    gfx_pitch = kernel_framebuffer_pitch();

    if (gfx_addr == 0 || gfx_width == 0 || gfx_height == 0 || gfx_pitch == 0) {
        gfx_ready = 0;
        return;
    }

    gfx_ready = 1;
}




int gfx_is_ready(void) {
    return gfx_ready;
}

void gfx_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    uint32_t *pixel = 0;
    uint32_t offset = 0;

    if (!gfx_ready) {
        return;
    }

    if (x >= gfx_width || y >= gfx_height) {
        return;
    }

    offset = y * gfx_pitch + x * 4;
    pixel = (uint32_t *)(gfx_addr + offset);
    *pixel = color;
}

void gfx_clear(uint32_t color) {
    if (!gfx_ready) {
        return;
    }

    for (uint32_t y = 0; y < gfx_height; y++) {
        for (uint32_t x = 0; x < gfx_width; x++) {
            gfx_put_pixel(x, y, color);
        }
    }
}

void gfx_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!gfx_ready) {
        return;
    }

    for (uint32_t yy = 0; yy < h; yy++) {
        for (uint32_t xx = 0; xx < w; xx++) {
            gfx_put_pixel(x + xx, y + yy, color);
        }
    }
}

static void gfx_draw_bar(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t progress) {
    uint32_t fill = 0;

    if (progress > 100) {
        progress = 100;
    }

    fill = (w * progress) / 100;

    gfx_fill_rect(x, y, w, h, rgb(30, 36, 56));
    gfx_fill_rect(x, y, fill, h, rgb(90, 180, 255));
}

static void gfx_draw_logo_blocks(uint32_t cx, uint32_t y) {
    uint32_t c = rgb(235, 240, 255);

    /*
     * Blocky LOS logo.
     * This is intentionally primitive: pure rectangles, no font dependency.
     */

    /* L */
    gfx_fill_rect(cx + 0, y + 0, 16, 110, c);
    gfx_fill_rect(cx + 0, y + 94, 70, 16, c);

    /* O */
    gfx_fill_rect(cx + 100, y + 0, 70, 16, c);
    gfx_fill_rect(cx + 100, y + 94, 70, 16, c);
    gfx_fill_rect(cx + 100, y + 0, 16, 110, c);
    gfx_fill_rect(cx + 154, y + 0, 16, 110, c);

    /* S */
    gfx_fill_rect(cx + 200, y + 0, 70, 16, c);
    gfx_fill_rect(cx + 200, y + 47, 70, 16, c);
    gfx_fill_rect(cx + 200, y + 94, 70, 16, c);
    gfx_fill_rect(cx + 200, y + 0, 16, 55, c);
    gfx_fill_rect(cx + 254, y + 47, 16, 63, c);
}

void gfx_draw_boot_screen(void) {
    uint32_t bg = rgb(8, 10, 18);
    uint32_t panel = rgb(16, 20, 34);
    uint32_t border = rgb(65, 78, 110);
    uint32_t accent = rgb(90, 180, 255);

    if (!gfx_ready) {
        return;
    }

    gfx_clear(bg);

    uint32_t panel_x = gfx_width / 10;
    uint32_t panel_y = gfx_height / 10;
    uint32_t panel_w = gfx_width - panel_x * 2;
    uint32_t panel_h = gfx_height - panel_y * 2;

    gfx_fill_rect(panel_x, panel_y, panel_w, panel_h, panel);
    gfx_fill_rect(panel_x, panel_y, panel_w, 4, border);
    gfx_fill_rect(panel_x, panel_y + panel_h - 4, panel_w, 4, border);
    gfx_fill_rect(panel_x, panel_y, 4, panel_h, border);
    gfx_fill_rect(panel_x + panel_w - 4, panel_y, 4, panel_h, border);

    gfx_draw_logo_blocks((gfx_width - 270) / 2, panel_y + 70);

    /* Subtitle underline */
    gfx_fill_rect((gfx_width - 360) / 2, panel_y + 220, 360, 4, accent);

    /* Loading bars */
    uint32_t bx = panel_x + 120;
    uint32_t by = panel_y + 285;
    uint32_t bw = panel_w - 240;
    uint32_t bh = 18;

    gfx_draw_bar(bx, by + 0,   bw, bh, 35);
    gfx_draw_bar(bx, by + 40,  bw, bh, 55);
    gfx_draw_bar(bx, by + 80,  bw, bh, 75);
    gfx_draw_bar(bx, by + 120, bw, bh, 90);
    gfx_draw_bar(bx, by + 160, bw, bh, 100);

    /* Bottom status block */
    gfx_fill_rect(panel_x + 80, panel_y + panel_h - 90, panel_w - 160, 36, rgb(22, 30, 48));
    gfx_fill_rect(panel_x + 80, panel_y + panel_h - 90, panel_w - 160, 4, accent);
}

static void gfx_frame(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    gfx_fill_rect(x, y, w, 3, color);
    gfx_fill_rect(x, y + h - 3, w, 3, color);
    gfx_fill_rect(x, y, 3, h, color);
    gfx_fill_rect(x + w - 3, y, 3, h, color);
}

static void gfx_draw_ring_blocks(uint32_t cx, uint32_t cy) {
    uint32_t outer = rgb(90, 180, 255);
    uint32_t inner = rgb(16, 20, 34);
    uint32_t glow = rgb(40, 90, 150);

    /* blocky ring / AI core */
    gfx_fill_rect(cx - 70, cy - 70, 140, 10, glow);
    gfx_fill_rect(cx - 70, cy + 60, 140, 10, glow);
    gfx_fill_rect(cx - 70, cy - 70, 10, 140, glow);
    gfx_fill_rect(cx + 60, cy - 70, 10, 140, glow);

    gfx_fill_rect(cx - 50, cy - 50, 100, 10, outer);
    gfx_fill_rect(cx - 50, cy + 40, 100, 10, outer);
    gfx_fill_rect(cx - 50, cy - 50, 10, 100, outer);
    gfx_fill_rect(cx + 40, cy - 50, 10, 100, outer);

    gfx_fill_rect(cx - 26, cy - 26, 52, 52, inner);
    gfx_fill_rect(cx - 12, cy - 12, 24, 24, outer);
}

static void gfx_draw_small_logo(uint32_t x, uint32_t y) {
    uint32_t c = rgb(235, 240, 255);

    /* L */
    gfx_fill_rect(x + 0, y + 0, 8, 44, c);
    gfx_fill_rect(x + 0, y + 36, 32, 8, c);

    /* O */
    gfx_fill_rect(x + 46, y + 0, 34, 8, c);
    gfx_fill_rect(x + 46, y + 36, 34, 8, c);
    gfx_fill_rect(x + 46, y + 0, 8, 44, c);
    gfx_fill_rect(x + 72, y + 0, 8, 44, c);

    /* S */
    gfx_fill_rect(x + 94, y + 0, 34, 8, c);
    gfx_fill_rect(x + 94, y + 18, 34, 8, c);
    gfx_fill_rect(x + 94, y + 36, 34, 8, c);
    gfx_fill_rect(x + 94, y + 0, 8, 24, c);
    gfx_fill_rect(x + 120, y + 18, 8, 26, c);
}


static void gfx_draw_hline(uint32_t x, uint32_t y, uint32_t w, uint32_t color) {
    if (!gfx_ready) return;
    for (uint32_t i = 0; i < w; i++) {
        gfx_put_pixel(x + i, y, color);
    }
}

static void gfx_draw_vline(uint32_t x, uint32_t y, uint32_t h, uint32_t color) {
    if (!gfx_ready) return;
    for (uint32_t i = 0; i < h; i++) {
        gfx_put_pixel(x, y + i, color);
    }
}

static void gfx_fill_circle(int cx, int cy, int r, uint32_t color) {
    if (!gfx_ready || r <= 0) return;

    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if ((x * x + y * y) <= (r * r)) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && py >= 0) {
                    gfx_put_pixel((uint32_t)px, (uint32_t)py, color);
                }
            }
        }
    }
}

static void gfx_draw_ring_circle(int cx, int cy, int r_outer, int thickness, uint32_t color) {
    if (!gfx_ready || r_outer <= 0 || thickness <= 0) return;

    int r_inner = r_outer - thickness;
    if (r_inner < 0) r_inner = 0;

    int outer2 = r_outer * r_outer;
    int inner2 = r_inner * r_inner;

    for (int y = -r_outer; y <= r_outer; y++) {
        for (int x = -r_outer; x <= r_outer; x++) {
            int d2 = x * x + y * y;
            if (d2 <= outer2 && d2 >= inner2) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && py >= 0) {
                    gfx_put_pixel((uint32_t)px, (uint32_t)py, color);
                }
            }
        }
    }
}




void gfx_set_ring_anchor(int anchor) {
    if (anchor < 0) {
        anchor = 0;
    }

    if (anchor > 1) {
        anchor = 1;
    }

    gfx_ring_anchor = anchor;
}

void gfx_set_model_state(int state) {
    if (state < 0) state = 0;
    if (state > 4) state = 4;

    gfx_model_state = state;
    gfx_anim_tick = 0;
}


int gfx_get_model_state(void) {
    return gfx_model_state;
}

static void gfx_draw_model_ring(void) {
    uint32_t bg = rgb(1, 4, 13);
    uint32_t blue = rgb(90, 170, 255);
    uint32_t pale = rgb(210, 230, 255);
    uint32_t dim = rgb(35, 55, 95);

    int screen_cx;
    int screen_cy;

    int cx = MODEL_RING_BOX / 2;
    int cy = MODEL_RING_BOX / 2;

    int radius;
    int thickness;
    int pulse = 0;

    if (!gfx_ready) {
        return;
    }

    /*
     * This animated renderer is now used only for the main AI surface.
     * Workspace has its own direct small ring renderer.
     */
    screen_cx = (int)(gfx_width / 2);
    screen_cy = (int)(gfx_height / 2) - 30;
    radius = 92;
    thickness = 8;

    gfx_buf_clear(bg);

    if (gfx_model_state == 0) {
        gfx_buf_draw_ring_circle(cx, cy, radius, thickness, dim);
        gfx_blit_model_ring_buffer(screen_cx, screen_cy);
        return;
    }

    if (gfx_model_state == 1) {
        pulse = (int)((gfx_anim_tick / 18000) % 18);
        if (pulse > 9) {
            pulse = 18 - pulse;
        }

        gfx_buf_draw_ring_circle(cx, cy, radius + pulse, thickness, blue);
        gfx_buf_draw_ring_circle(cx, cy, radius - 22, 4, dim);
        gfx_blit_model_ring_buffer(screen_cx, screen_cy);
        return;
    }

    if (gfx_model_state == 2) {
        gfx_buf_draw_ring_circle(cx, cy, radius, thickness, blue);
        gfx_buf_fill_circle(cx, cy, 8, blue);
        gfx_blit_model_ring_buffer(screen_cx, screen_cy);
        return;
    }

    if (gfx_model_state == 3) {
        pulse = (int)((gfx_anim_tick / 14000) % 24);
        if (pulse > 12) {
            pulse = 24 - pulse;
        }

        gfx_buf_draw_ring_circle(cx, cy, radius + pulse, 6, blue);
        gfx_buf_draw_ring_circle(cx, cy, radius - 24 - pulse / 2, 5, pale);
        gfx_buf_fill_circle(cx, cy, 10 + pulse / 3, blue);
        gfx_blit_model_ring_buffer(screen_cx, screen_cy);
        return;
    }

    if (gfx_model_state == 4) {
        pulse = (int)((gfx_anim_tick / 10000) % 30);
        if (pulse > 15) {
            pulse = 30 - pulse;
        }

        gfx_buf_draw_ring_circle(cx, cy, radius + pulse, 5, pale);
        gfx_buf_draw_ring_circle(cx, cy, radius - 20, 5, blue);
        gfx_buf_draw_ring_circle(cx, cy, radius - 44 + pulse / 2, 4, dim);
        gfx_buf_fill_circle(cx, cy, 7, pale);
        gfx_blit_model_ring_buffer(screen_cx, screen_cy);
        return;
    }
}

void gfx_draw_ai_surface(void) {
    uint32_t bg = rgb(1, 4, 13);

    if (!gfx_ready) {
        return;
    }

    gfx_set_ring_anchor(0);

    gfx_clear(bg);
    gfx_draw_model_ring();
    gfx_draw_shell_input("");
}







static uint8_t gfx_font_row(char ch, int row) {
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);

    switch (ch) {
        case 'A': { static const uint8_t r[7]={14,17,17,31,17,17,17}; return r[row]; }
        case 'B': { static const uint8_t r[7]={30,17,17,30,17,17,30}; return r[row]; }
        case 'C': { static const uint8_t r[7]={14,17,16,16,16,17,14}; return r[row]; }
        case 'D': { static const uint8_t r[7]={30,17,17,17,17,17,30}; return r[row]; }
        case 'E': { static const uint8_t r[7]={31,16,16,30,16,16,31}; return r[row]; }
        case 'F': { static const uint8_t r[7]={31,16,16,30,16,16,16}; return r[row]; }
        case 'G': { static const uint8_t r[7]={14,17,16,23,17,17,14}; return r[row]; }
        case 'H': { static const uint8_t r[7]={17,17,17,31,17,17,17}; return r[row]; }
        case 'I': { static const uint8_t r[7]={14,4,4,4,4,4,14}; return r[row]; }
        case 'J': { static const uint8_t r[7]={1,1,1,1,17,17,14}; return r[row]; }
        case 'K': { static const uint8_t r[7]={17,18,20,24,20,18,17}; return r[row]; }
        case 'L': { static const uint8_t r[7]={16,16,16,16,16,16,31}; return r[row]; }
        case 'M': { static const uint8_t r[7]={17,27,21,21,17,17,17}; return r[row]; }
        case 'N': { static const uint8_t r[7]={17,25,21,19,17,17,17}; return r[row]; }
        case 'O': { static const uint8_t r[7]={14,17,17,17,17,17,14}; return r[row]; }
        case 'P': { static const uint8_t r[7]={30,17,17,30,16,16,16}; return r[row]; }
        case 'Q': { static const uint8_t r[7]={14,17,17,17,21,18,13}; return r[row]; }
        case 'R': { static const uint8_t r[7]={30,17,17,30,20,18,17}; return r[row]; }
        case 'S': { static const uint8_t r[7]={15,16,16,14,1,1,30}; return r[row]; }
        case 'T': { static const uint8_t r[7]={31,4,4,4,4,4,4}; return r[row]; }
        case 'U': { static const uint8_t r[7]={17,17,17,17,17,17,14}; return r[row]; }
        case 'V': { static const uint8_t r[7]={17,17,17,17,17,10,4}; return r[row]; }
        case 'W': { static const uint8_t r[7]={17,17,17,21,21,21,10}; return r[row]; }
        case 'X': { static const uint8_t r[7]={17,17,10,4,10,17,17}; return r[row]; }
        case 'Y': { static const uint8_t r[7]={17,17,10,4,4,4,4}; return r[row]; }
        case 'Z': { static const uint8_t r[7]={31,1,2,4,8,16,31}; return r[row]; }

        case '0': { static const uint8_t r[7]={14,17,19,21,25,17,14}; return r[row]; }
        case '1': { static const uint8_t r[7]={4,12,4,4,4,4,14}; return r[row]; }
        case '2': { static const uint8_t r[7]={14,17,1,2,4,8,31}; return r[row]; }
        case '3': { static const uint8_t r[7]={30,1,1,14,1,1,30}; return r[row]; }
        case '4': { static const uint8_t r[7]={2,6,10,18,31,2,2}; return r[row]; }
        case '5': { static const uint8_t r[7]={31,16,16,30,1,1,30}; return r[row]; }
        case '6': { static const uint8_t r[7]={14,16,16,30,17,17,14}; return r[row]; }
        case '7': { static const uint8_t r[7]={31,1,2,4,8,8,8}; return r[row]; }
        case '8': { static const uint8_t r[7]={14,17,17,14,17,17,14}; return r[row]; }
        case '9': { static const uint8_t r[7]={14,17,17,15,1,1,14}; return r[row]; }

        case ':': { static const uint8_t r[7]={0,4,4,0,4,4,0}; return r[row]; }
        case '.': { static const uint8_t r[7]={0,0,0,0,0,12,12}; return r[row]; }
        case '-': { static const uint8_t r[7]={0,0,0,31,0,0,0}; return r[row]; }
        case '_': { static const uint8_t r[7]={0,0,0,0,0,0,31}; return r[row]; }
        case '/': { static const uint8_t r[7]={1,1,2,4,8,16,16}; return r[row]; }
        case '>': { static const uint8_t r[7]={16,8,4,2,4,8,16}; return r[row]; }
        case '=': { static const uint8_t r[7]={0,31,0,0,31,0,0}; return r[row]; }
        case '?': { static const uint8_t r[7]={14,17,1,2,4,0,4}; return r[row]; }
        case ' ': return 0;
    }

    return 0;
}

static void gfx_draw_char(uint32_t x, uint32_t y, char ch, uint32_t color, uint32_t scale) {
    if (scale == 0) scale = 1;

    for (int row = 0; row < 7; row++) {
        uint8_t bits = gfx_font_row(ch, row);

        for (int col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                gfx_fill_rect(
                    x + (uint32_t)col * scale,
                    y + (uint32_t)row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

void gfx_draw_text(uint32_t x, uint32_t y, const char *text, uint32_t color, uint32_t scale) {
    uint32_t cx = x;

    if (!text) return;
    if (scale == 0) scale = 1;

    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            y += 9 * scale;
            cx = x;
            continue;
        }

        gfx_draw_char(cx, y, text[i], color, scale);
        cx += 6 * scale;
    }
}



void gfx_draw_shell_surface(void) {
    uint32_t bg = rgb(5, 7, 12);
    uint32_t panel2 = rgb(20, 26, 44);
    uint32_t border = rgb(58, 72, 108);
    uint32_t accent = rgb(90, 180, 255);
    uint32_t text = rgb(235, 240, 255);
    uint32_t muted = rgb(120, 135, 165);

    if (!gfx_ready) {
        return;
    }

    gfx_surface_mode = 1;
    gfx_clear(bg);

    uint32_t margin = 38;
    uint32_t top = 32;
    uint32_t bottom = gfx_height - 42;

    gfx_frame(margin, top, gfx_width - margin * 2, bottom - top, border);

    gfx_fill_rect(margin + 3, top + 3, gfx_width - margin * 2 - 6, 56, panel2);
    gfx_draw_small_logo(margin + 28, top + 12);
    gfx_fill_rect(margin + 190, top + 28, gfx_width - margin * 2 - 240, 4, accent);
    gfx_draw_text(margin + 210, top + 25, "SHELL MODE", text, 2);

    gfx_draw_text(margin + 42, top + 92, "FRAMEBUFFER SHELL", text, 2);
    gfx_draw_text(margin + 42, top + 128, "TYPE COMMANDS BELOW.", muted, 1);
    gfx_draw_text(margin + 42, top + 148, "USE /AI TO RETURN TO AI SURFACE.", muted, 1);

    gfx_frame(margin + 42, top + 188, gfx_width - margin * 2 - 84, bottom - top - 288, border);
    gfx_draw_text(margin + 64, top + 212, "OUTPUT WILL BE ROUTED HERE NEXT.", muted, 1);

    gfx_draw_shell_input("");
}

void gfx_draw_shell_input(const char *input) {
    uint32_t bg = rgb(1, 4, 13);
    uint32_t cursor = rgb(90, 170, 255);
    uint32_t text = rgb(90, 170, 255);

    uint32_t base_x = 42;
    uint32_t base_y = gfx_height - 58;
    uint32_t cursor_x = base_x;

    if (!gfx_ready) {
        return;
    }

    /*
     * Clear bottom input zone.
     * No prompt, no frame, no labels.
     */
    gfx_fill_rect(0, base_y - 14, gfx_width, 48, bg);

    /*
     * Draw typed input only.
     */
    if (input && input[0]) {
        gfx_draw_text(base_x, base_y, input, text, 2);

        for (int i = 0; input[i]; i++) {
            cursor_x += 12;
        }
    }

    if (cursor_x > gfx_width - 24) {
        cursor_x = gfx_width - 24;
    }

    /*
     * Cursor after current text.
     */
    gfx_fill_rect(cursor_x, base_y + 2, 10, 24, cursor);
}




static uint32_t gfx_key_tick = 0;

void gfx_debug_key_tick(void) {
    if (!gfx_ready) return;

    gfx_key_tick++;

    uint32_t x = gfx_width - 80;
    uint32_t y = 92;
    uint32_t w = 8 + (gfx_key_tick % 8) * 8;

    gfx_fill_rect(x, y, 72, 10, rgb(8, 12, 22));
    gfx_fill_rect(x, y, w, 10, rgb(90, 180, 255));
}


void gfx_redraw_current_surface(void) {
    if (!gfx_ready) return;

    if (gfx_surface_mode == 1) {
        gfx_draw_shell_surface();
    } else {
        gfx_draw_ai_surface();
    }
}

void gfx_draw_status_line(const char *text) {
    (void)text;
    /*
     * Ultra-minimal surface: no status text.
     */
}


static void gfx_draw_mouse_cursor_shape(void) {
    if (!gfx_ready) return;

    uint32_t c = rgb(235, 240, 255);
    uint32_t a = rgb(90, 180, 255);

    uint32_t x = (uint32_t)gfx_mouse_x;
    uint32_t y = (uint32_t)gfx_mouse_y;

    gfx_fill_rect(x, y, 2, 14, c);
    gfx_fill_rect(x + 2, y + 2, 2, 12, c);
    gfx_fill_rect(x + 4, y + 4, 2, 10, c);
    gfx_fill_rect(x + 6, y + 6, 2, 8, c);
    gfx_fill_rect(x + 8, y + 8, 2, 6, c);
    gfx_fill_rect(x + 10, y + 10, 2, 4, c);

    if (gfx_mouse_buttons & 1) {
        gfx_fill_rect(x + 14, y, 8, 8, a);
    }
}

void gfx_mouse_move(int dx, int dy, uint8_t buttons) {
    if (!gfx_ready) return;

    gfx_mouse_x += dx;
    gfx_mouse_y -= dy;
    gfx_mouse_buttons = buttons;

    if (gfx_mouse_x < 0) gfx_mouse_x = 0;
    if (gfx_mouse_y < 0) gfx_mouse_y = 0;
    if (gfx_mouse_x > (int)gfx_width - 24) gfx_mouse_x = (int)gfx_width - 24;
    if (gfx_mouse_y > (int)gfx_height - 24) gfx_mouse_y = (int)gfx_height - 24;

    gfx_redraw_current_surface();
    gfx_draw_mouse_cursor_shape();
}

void gfx_draw_boot_selector(void) {
    uint32_t bg = rgb(7, 9, 16);
    uint32_t border = rgb(58, 72, 108);
    uint32_t accent = rgb(90, 180, 255);
    uint32_t text = rgb(235, 240, 255);
    uint32_t muted = rgb(120, 135, 165);

    if (!gfx_ready) return;

    gfx_clear(bg);

    uint32_t x = 90;
    uint32_t y = 90;
    uint32_t w = gfx_width - 180;
    uint32_t h = gfx_height - 180;

    gfx_frame(x, y, w, h, border);
    gfx_draw_small_logo(x + 34, y + 32);
    gfx_draw_text(x + 210, y + 52, "BOOT SELECTOR", text, 2);
    gfx_fill_rect(x + 210, y + 82, w - 260, 4, accent);

    gfx_draw_text(x + 70, y + 150, "SELECT STARTUP SURFACE", text, 2);

    gfx_frame(x + 70, y + 210, w - 140, 58, border);
    gfx_draw_text(x + 92, y + 230, "F1 / 1", accent, 2);
    gfx_draw_text(x + 210, y + 230, "AI SURFACE", text, 2);

    gfx_frame(x + 70, y + 292, w - 140, 58, border);
    gfx_draw_text(x + 92, y + 312, "F2 / 2", accent, 2);
    gfx_draw_text(x + 210, y + 312, "LEGACY SHELL / NORTON TARGET", text, 2);

    gfx_draw_text(x + 70, y + h - 70, "DEFAULT: AI SURFACE. THIS SCREEN IS KERNEL-LEVEL.", muted, 1);
}

void gfx_draw_legacy_target_surface(void) {
    uint32_t bg = rgb(5, 7, 12);
    uint32_t border = rgb(58, 72, 108);
    uint32_t accent = rgb(90, 180, 255);
    uint32_t text = rgb(235, 240, 255);
    uint32_t muted = rgb(120, 135, 165);

    if (!gfx_ready) return;

    gfx_clear(bg);

    uint32_t margin = 38;
    uint32_t top = 32;
    uint32_t bottom = gfx_height - 42;

    gfx_frame(margin, top, gfx_width - margin * 2, bottom - top, border);
    gfx_draw_small_logo(margin + 28, top + 12);
    gfx_fill_rect(margin + 190, top + 28, gfx_width - margin * 2 - 240, 4, accent);
    gfx_draw_text(margin + 210, top + 25, "LEGACY TARGET", text, 2);

    gfx_draw_text(margin + 52, top + 110, "NORTON / SHELL / EDITOR TARGET", text, 2);
    gfx_draw_text(margin + 52, top + 150, "NEXT STEP: FRAMEBUFFER TERMINAL ADAPTER.", muted, 1);
    gfx_draw_text(margin + 52, top + 170, "AFTER THAT OLD TEXT UI WILL RENDER HERE.", muted, 1);
    gfx_draw_text(margin + 52, top + 210, "COMMANDS:", text, 1);
    gfx_draw_text(margin + 52, top + 232, "/AI     RETURN TO AI SURFACE", muted, 1);
    gfx_draw_text(margin + 52, top + 252, "/BOOT   SHOW BOOT SELECTOR", muted, 1);

    gfx_draw_shell_input("");
}





/*
 * Generated workspace state.
 */
static int gfx_workspace_layout = 0; /* 0 empty, 1 coding, 2 debug */
static uint32_t gfx_workspace_widgets = 0;

#define GFX_WIDGET_TERMINAL  (1u << 0)
#define GFX_WIDGET_EDITOR    (1u << 1)
#define GFX_WIDGET_FILES     (1u << 2)
#define GFX_WIDGET_BROWSER   (1u << 3)
#define GFX_WIDGET_LOGS      (1u << 4)
#define GFX_WIDGET_DOCKER    (1u << 5)
#define GFX_WIDGET_SSH       (1u << 6)

static int gfx_streq(const char *a, const char *b) {
    int i = 0;

    if (!a || !b) {
        return 0;
    }

    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

void gfx_workspace_reset(void) {
    gfx_workspace_layout = 0;
    gfx_workspace_widgets = 0;
}

void gfx_workspace_set_layout(const char *layout) {
    if (gfx_streq(layout, "coding")) {
        gfx_workspace_layout = 1;
        return;
    }

    if (gfx_streq(layout, "debug") || gfx_streq(layout, "debugging")) {
        gfx_workspace_layout = 2;
        return;
    }

    gfx_workspace_layout = 0;
}

void gfx_workspace_add_widget(const char *widget) {
    if (gfx_streq(widget, "terminal")) {
        gfx_workspace_widgets |= GFX_WIDGET_TERMINAL;
        return;
    }

    if (gfx_streq(widget, "editor")) {
        gfx_workspace_widgets |= GFX_WIDGET_EDITOR;
        return;
    }

    if (gfx_streq(widget, "files")) {
        gfx_workspace_widgets |= GFX_WIDGET_FILES;
        return;
    }

    if (gfx_streq(widget, "browser")) {
        gfx_workspace_widgets |= GFX_WIDGET_BROWSER;
        return;
    }

    if (gfx_streq(widget, "logs")) {
        gfx_workspace_widgets |= GFX_WIDGET_LOGS;
        return;
    }

    if (gfx_streq(widget, "docker")) {
        gfx_workspace_widgets |= GFX_WIDGET_DOCKER;
        return;
    }

    if (gfx_streq(widget, "ssh")) {
        gfx_workspace_widgets |= GFX_WIDGET_SSH;
        return;
    }
}

static void gfx_draw_widget_panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *title, const char *hint) {
    uint32_t bg = rgb(1, 4, 13);
    uint32_t border = rgb(35, 55, 95);
    uint32_t fg = rgb(90, 170, 255);
    uint32_t dim = rgb(35, 55, 95);

    /*
     * Very dark panel. Not a heavy window, just generated surface area.
     */
    gfx_fill_rect(x, y, w, h, bg);

    gfx_draw_hline(x, y, w, border);
    gfx_draw_hline(x, y + h - 1, w, border);
    gfx_draw_vline(x, y, h, border);
    gfx_draw_vline(x + w - 1, y, h, border);

    gfx_draw_text(x + 12, y + 12, title, fg, 1);

    if (hint) {
        gfx_draw_text(x + 12, y + 34, hint, dim, 1);
    }
}



static void gfx_draw_workspace_status_ring(void) {
    uint32_t ring = rgb(90, 170, 255);

    uint32_t cx;
    uint32_t cy;

    if (!gfx_ready) {
        return;
    }

    /*
     * Workspace ring:
     * direct draw, no offscreen background, no center dot.
     */
    cx = gfx_width - 112;
    cy = 74;

    gfx_draw_ring_circle((int)cx, (int)cy, 34, 4, ring);
}


void gfx_draw_workspace_surface(void) {
    uint32_t bg = rgb(1, 4, 13);
    uint32_t dim = rgb(35, 55, 95);
    uint32_t fg = rgb(90, 170, 255);

    uint32_t margin = 48;
    uint32_t header_y = 48;
    uint32_t area_x = margin;
    uint32_t area_y = 118;
    uint32_t area_w = gfx_width - margin * 2;
    uint32_t area_h = gfx_height - area_y - 118;

    if (!gfx_ready) {
        return;
    }

    gfx_set_ring_anchor(1);

    gfx_clear(bg);

    /*
     * Workspace header.
     * AI ring is now a small status element in the top-right corner.
     */
    if (gfx_workspace_layout == 1) {
        gfx_draw_text(margin, header_y, "WORKSPACE / CODING", fg, 1);
    } else if (gfx_workspace_layout == 2) {
        gfx_draw_text(margin, header_y, "WORKSPACE / DEBUG", fg, 1);
    } else {
        gfx_draw_text(margin, header_y, "WORKSPACE", fg, 1);
    }

    gfx_draw_hline(margin, 86, area_w, dim);

    /*
     * Draw the small status ring after clearing the screen.
     */
    gfx_draw_workspace_status_ring();

    if (gfx_workspace_layout == 1) {
        uint32_t left_w = (area_w * 62) / 100;
        uint32_t right_w = area_w - left_w - 16;
        uint32_t top_h = (area_h * 62) / 100;
        uint32_t bottom_h = area_h - top_h - 16;
        uint32_t right_x = area_x + left_w + 16;

        if (gfx_workspace_widgets & GFX_WIDGET_EDITOR) {
            gfx_draw_widget_panel(area_x, area_y, left_w, top_h, "EDITOR", "generated code surface");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_TERMINAL) {
            gfx_draw_widget_panel(area_x, area_y + top_h + 16, left_w, bottom_h, "TERMINAL", "shell / commands / build output");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_FILES) {
            gfx_draw_widget_panel(right_x, area_y, right_w, (area_h - 16) / 2, "FILES", "project tree");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_LOGS) {
            gfx_draw_widget_panel(right_x, area_y + ((area_h - 16) / 2) + 16, right_w, (area_h - 16) / 2, "LOGS", "runtime messages");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_DOCKER) {
            gfx_draw_widget_panel(right_x, area_y, right_w, (area_h - 16) / 2, "DOCKER", "containers / services");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_SSH) {
            gfx_draw_widget_panel(right_x, area_y + ((area_h - 16) / 2) + 16, right_w, (area_h - 16) / 2, "SSH", "remote sessions");
        }

        if ((gfx_workspace_widgets & (GFX_WIDGET_FILES | GFX_WIDGET_LOGS | GFX_WIDGET_DOCKER | GFX_WIDGET_SSH | GFX_WIDGET_BROWSER)) == 0) {
            gfx_draw_widget_panel(right_x, area_y, right_w, area_h, "AI CONTEXT", "next generated widgets");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_BROWSER) {
            gfx_draw_widget_panel(right_x, area_y, right_w, area_h, "BROWSER", "web / docs / preview");
        }
    } else if (gfx_workspace_layout == 2) {
        uint32_t col_w = (area_w - 32) / 3;

        if (gfx_workspace_widgets & GFX_WIDGET_LOGS) {
            gfx_draw_widget_panel(area_x, area_y, col_w, area_h, "LOGS", "debug stream");
        } else {
            gfx_draw_widget_panel(area_x, area_y, col_w, area_h, "STATE", "runtime inspection");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_TERMINAL) {
            gfx_draw_widget_panel(area_x + col_w + 16, area_y, col_w, area_h, "TERMINAL", "diagnostics");
        }

        if (gfx_workspace_widgets & GFX_WIDGET_DOCKER) {
            gfx_draw_widget_panel(area_x + (col_w + 16) * 2, area_y, col_w, area_h, "DOCKER", "containers");
        } else if (gfx_workspace_widgets & GFX_WIDGET_SSH) {
            gfx_draw_widget_panel(area_x + (col_w + 16) * 2, area_y, col_w, area_h, "SSH", "remote debug");
        } else {
            gfx_draw_widget_panel(area_x + (col_w + 16) * 2, area_y, col_w, area_h, "WATCH", "signals / events");
        }
    } else {
        gfx_draw_hline(area_x, area_y, area_w, dim);
        gfx_draw_hline(area_x, area_y + area_h, area_w, dim);
        gfx_draw_text(area_x + 12, area_y + 24, "READY", dim, 1);
        gfx_draw_text(area_x + 12, area_y + 46, "ASK MODEL TO GENERATE WIDGETS", dim, 1);
    }

    gfx_draw_shell_input("");
}




void gfx_tick(void) {
    if (!gfx_ready) {
        return;
    }

    /*
     * Workspace has its own clean static ring.
     * Do not redraw animated ring buffer over workspace,
     * otherwise it can overwrite panels with a black square.
     */
    if (gfx_ring_anchor == 1) {
        return;
    }

    /*
     * Idle is fully static.
     */
    if (gfx_model_state == 0) {
        return;
    }

    gfx_anim_tick++;

    /*
     * AI surface animation only.
     */
    if ((gfx_anim_tick % 9000) == 0) {
        gfx_draw_model_ring();
    }
}





void gfx_draw_host_note(const char *text) {
    uint32_t bg = rgb(1, 4, 13);
    uint32_t fg = rgb(90, 170, 255);
    uint32_t dim = rgb(35, 55, 95);

    uint32_t x = 64;
    uint32_t y = gfx_height - 150;
    uint32_t w = gfx_width - 128;

    if (!gfx_ready || !text) {
        return;
    }

    /*
     * Minimal model response area.
     * It appears only after host/model sends a note.
     */
    gfx_fill_rect(0, y - 20, gfx_width, 70, bg);
    gfx_draw_hline(x, y - 12, w, dim);

    gfx_draw_text(x, y, text, fg, 1);
}
