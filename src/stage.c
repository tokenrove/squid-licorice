/* stage represents the scrolling background
 * it's a singleton because fuck you.
 */

#include "stage.h"
#include "tilemap.h"
#include "geometry.h"
#include "camera.h"
#include "ensure.h"

static struct tilemap t;
static position tm_p = 0.;

void stage_start(void)
{
    ENSURE(tilemap_load("map", "atlas.png", &t));
}

void stage_end(void)
{
    tilemap_destroy(&t);
}

void stage_draw(void)
{
    tilemap_draw(&t, tm_p);
}

position scroll_speed = 50.f*I;

void stage_update(float elapsed_time)
{
    world_camera.translation += elapsed_time * scroll_speed;
}
