#pragma once

typedef enum {
    MSG_ENTER,
    MSG_EXIT,
    MSG_TICK,
    /* the following could go in game_constants.h instead */
    MSG_OFFSIDE,
    MSG_COLLISION,  /* see physics.h */
    MSG_DAMAGE,     /* see projectile.h */
    MSG_USER        /* add more types from here */
} msg_type;

struct msg {
    msg_type type;
};

struct tick_msg {
    struct msg super;
    float elapsed_time;
};

struct ear;
enum handler_return { STATE_IGNORED, STATE_HANDLED, STATE_TRANSITION };
/* messages are usually constructed on the stack, so handlers should
 * always copy anything they need to hold on to after returning. */
typedef enum handler_return (*msg_handler)(struct ear *, struct msg *);

struct ear {
    msg_handler handler;
};

extern void ear_tell(struct ear *, struct msg *);
#define TELL(E, M) (ear_tell((struct ear *)(E), (struct msg *)(M)))
