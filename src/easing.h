#pragma once

typedef float (*easing_fn)(float, float);

extern float ease_float(float start, float end, float elapsed, float duration, easing_fn fn);
extern float easing_linear(float elapsed, float duration);
extern float easing_cubic(float elapsed, float duration);
extern float easing_elastic(float elapsed, float duration);
