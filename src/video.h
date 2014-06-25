#pragma once

#include <stdint.h>

extern uint16_t viewport_w, viewport_h;

extern void video_init(void);
extern void video_start_frame(void);
extern void video_end_frame(void);

extern void video_save_fb_screenshot(const char *path);

