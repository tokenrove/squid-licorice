#pragma once

#include "msg.h"

#define TELL(E, M) (ear_tell((struct ear *)(E), (struct msg *)(M)))
#define XITION(H) (me->base.handler = (msg_handler)(H), STATE_TRANSITION)
