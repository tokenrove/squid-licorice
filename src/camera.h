#pragma once

#include "geometry.h"

struct camera {
    position translation;
    float scaling, rotation;
};

extern void camera_init(void);

// column-oriented
extern struct camera osd_camera, world_camera;
extern float camera_projection_matrix[16];
