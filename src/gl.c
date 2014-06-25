#include <GL/glew.h>
#include <GL/gl.h>

#include "gl.h"
#include "ensure.h"

void gl_init(void)
{
    ENSURE(GLEW_OK == glewInit());

    // XXX parameterize behavior by extensions supported here
    glDisable(GL_DEPTH_TEST);
    glClearColor(.8, .6, .5, 1.);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
