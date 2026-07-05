#ifndef LOS_GFX_H
#define LOS_GFX_H

#include <stdint.h>

void gfx_initialize_from_kernel(void);
int gfx_is_ready(void);

void gfx_clear(uint32_t color);
void gfx_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void gfx_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

void gfx_draw_boot_screen(void);
void gfx_draw_ai_surface(void);
void gfx_draw_workspace_surface(void);
void gfx_draw_boot_selector(void);
void gfx_draw_legacy_target_surface(void);
void gfx_draw_shell_surface(void);
void gfx_redraw_current_surface(void);

void gfx_draw_text(uint32_t x, uint32_t y, const char *text, uint32_t color, uint32_t scale);
void gfx_draw_shell_input(const char *input);
void gfx_draw_status_line(const char *text);
void gfx_debug_key_tick(void);

void gfx_mouse_move(int dx, int dy, uint8_t buttons);

void gfx_tick(void);

void gfx_set_model_state(int state);
int gfx_get_model_state(void);
void gfx_draw_host_note(const char *text);

void gfx_workspace_reset(void);
void gfx_workspace_set_layout(const char *layout);
void gfx_workspace_add_widget(const char *widget);
void gfx_set_ring_anchor(int anchor);

void gfx_begin_frame(void);
void gfx_present(void);
void gfx_end_frame(void);
#endif
