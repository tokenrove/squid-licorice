#include <math.h>
#include "easing.h"

float ease_float(float start, float end, float elapsed, float duration, easing_fn fn)
{
    return (end-start) * fn(elapsed, duration) + start;
}

float easing_linear(float elapsed, float duration)
{
    return elapsed / duration;
}

float easing_cubic(float elapsed, float duration)
{
    float f = elapsed / duration;
    return f * f * f;
}

#define TAU (2.f*(float)M_PI)
float easing_elastic(float elapsed, float duration)
{
    const float period = 0.42, amplitude = 1.5, decay = period / TAU * asinf(1.f/amplitude);
    float f = (elapsed / duration) - 1.f;
    float x = amplitude * powf(2., 10.f*f);
    x *= sinf(((elapsed*duration) - decay) * (TAU / period));
    return -x;
}
