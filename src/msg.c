#include "msg.h"
#include <stddef.h>

void ear_tell(struct ear *ear, struct msg *msg)
{
    msg_handler s = ear->handler;
    if (NULL == s) return;
    enum handler_return r = (*s)(ear, msg);
    if (STATE_TRANSITION != r) return;

    struct msg exit = {.type = MSG_EXIT},
              enter = {.type = MSG_ENTER};
    (*s)(ear, &exit);
    if (NULL == ear->handler) return;
    (ear->handler)(ear, &enter);
    // XXX deal with return value of last handler?
}



#ifdef UNIT_TEST_MSG
#include "libtap/tap.h"
#include <stdbool.h>

static void simple_message_passing_test(void)
{
    struct testing_ear {
        struct ear base;
        bool was_called;
    } a = {.base = {0}, .was_called = false};
    enum handler_return fn(struct testing_ear *me, struct msg *m) {
        if (m->type != MSG_USER) return STATE_IGNORED;
        me->was_called = true;
        return STATE_HANDLED;
    }
    a.base.handler = (msg_handler)fn;
    struct msg m = {.type = MSG_ENTER};
    cmp_ok(a.was_called, "==", false);
    TELL(&a, &m);
    m.type = MSG_USER;
    TELL(&a, &m);
    cmp_ok(a.was_called, "==", true);
}

int main(void)
{
    plan(2);
    simple_message_passing_test();
    // TODO verify hierarchical signal handling
    done_testing();
}

#endif
