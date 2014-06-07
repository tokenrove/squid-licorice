/* stage keeps track of the scrolling background layers;
 * it's a singleton because fuck you.
 */

#include "stage.h"
#include "layer.h"
#include "ensure.h"

/* Layers aren't being added and removed all the time, and there are
 * never many of them, so we just use an array here.
 */
enum { MAX_N_LAYERS = 8 };  /* arbitrary */
static struct layer *layers[MAX_N_LAYERS];
static int n_layers;

void stage_start(void)
{
    n_layers = 0;
}

void stage_end(void)
{
    memset(layers, 0, sizeof (layers));
    n_layers = 0;
}

void stage_add_layer(struct layer *layer)
{
    ENSURE(n_layers < MAX_N_LAYERS);
    layers[n_layers++] = layer;
}

void stage_remove_layer(struct layer *layer)
{
    ENSURE(n_layers > 0);
    int i;
    for (i = 0; i < n_layers && layers[i] != layer; ++i);
    ENSURE(i < n_layers);
    --n_layers;
    memmove(&layers[i], &layers[i+1], (n_layers-i) * sizeof(*layers));
}

void stage_draw(void)
{
    for (int i = 0; i < n_layers; ++i)
        layer_draw(layers[i]);
}

void stage_update(float elapsed_time)
{
    for (int i = 0; i < n_layers; ++i)
        layer_update(layers[i], elapsed_time);
}
