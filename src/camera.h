#pragma once

extern void camera_set_ortho_projection(float width, float height);

// column-oriented
extern float camera_projection_matrix[16];
extern float camera_screen_mv_matrix[16];
extern float camera_world_mv_matrix[16];
