#pragma once

typedef enum { IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT, IN_SHOOT, IN_MENU, IN_QUIT,
               IN_LAST } input;
typedef enum { NOT_PRESSED = 0, JUST_PRESSED, PRESSED } input_state;
extern input_state inputs[IN_LAST];

extern void input_init(void);
extern void input_update(void);
extern const char *input_get_readable_event_name_for_binding(input i);
extern void input_rebind_and_save(input i);
