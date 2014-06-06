/*
 * Lots of singletons.  Don't like it?  Don't buy it.
 */

#include <string.h>
#include <GL/gl.h>

#include "camera.h"
#include "video.h"

struct camera osd_camera = {.translation = 0., .scaling = 1., .rotation = 0.},
    world_camera = {.translation = 0., .scaling = 1., .rotation = 0.};

// NB: column-oriented!
GLfloat camera_projection_matrix[16] = {
    1., 0., 0., 0.,
    0., -1., 0., 0.,
    0., 0., -1., 0.,
    -1., 1., -1., 1.
};


static void camera_set_ortho_projection(float width, float height)
{
    camera_projection_matrix[0] = 2.f / width;
    camera_projection_matrix[5] = -2.f / height;
}

void camera_init(void)
{
    video_init();
    camera_set_ortho_projection(viewport_w, viewport_h);
}
