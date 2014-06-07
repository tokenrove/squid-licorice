#pragma once

#include "geometry.h"
#include "easing.h"
#include "layer.h"

extern void stage_start(void);
extern void stage_add_layer(struct layer *);
extern void stage_remove_layer(struct layer *);
extern void stage_end(void);
extern void stage_draw(void);
extern void stage_update(float elapsed_time);
