/*
 * Lots of singletons.  Don't like it?  Don't buy it.
 */

#include <string.h>
#include <GL/gl.h>

#include "camera.h"


// NB: column-oriented!
GLfloat camera_screen_mv_matrix[16] = {
    1., 0., 0., 0.,
    0., 1., 0., 0.,
    0., 0., 1., 0.,
    0., 0., 0., 1.
};

GLfloat camera_world_mv_matrix[16] = {
    1., 0., 0., 0.,
    0., 1., 0., 0.,
    0., 0., 1., 0.,
    0., 0., 0., 1.
};

GLfloat camera_projection_matrix[16] = {
    1., 0., 0., 0.,
    0., -1., 0., 0.,
    0., 0., -1., 0.,
    -1., 1., -1., 1.
};


void camera_set_ortho_projection(float width, float height)
{
    GLfloat m[16] = {
        2.f/width, 0., 0., 0.,
        0., -2.f/height, 0., 0.,
        0., 0., -1., 0.,
        -1., 1., -1., 1.
    };
    memcpy(camera_projection_matrix, m, sizeof (m));
}
