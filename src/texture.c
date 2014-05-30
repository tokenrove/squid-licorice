
#include <stdbool.h>

#include <pnglite.h>

#include <GL/gl.h>

#include "ensure.h"
#include "texture.h"

void texture_init(void)
{
    static bool initp = false;
    if (initp) return;
    initp = true;
    png_init(NULL, NULL);
}

struct texture texture_from_png(const char *path)
{
    struct texture t = { 0, .id = -1 };

    png_t png;
    ENSURE(PNG_NO_ERROR == png_open_file(&png, path));
    uint8_t *pels;
    pels = malloc(png.width*png.height*png.bpp);
    ENSURE(pels);
    ENSURE(PNG_NO_ERROR == png_get_data(&png, pels));

    t.width = png.width;
    t.height = png.height;

    glGenTextures(1, &t.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t.id);
    GLuint mode = (png.color_type == PNG_TRUECOLOR_ALPHA) ? GL_RGBA : GL_RGB;
    ENSURE(png.color_type == PNG_TRUECOLOR_ALPHA ||
           png.color_type == PNG_TRUECOLOR);
    glTexImage2D(GL_TEXTURE_2D, 0, mode, png.width, png.height, 0, mode, GL_UNSIGNED_BYTE, pels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    png_close_file(&png);
    free(pels);

    return t;
}
