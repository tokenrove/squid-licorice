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

#ifdef UNIT_TEST_EASING
#include "libtap/tap.h"
#include <time.h>

#define EPSILON 0.0001

int main(void)
{
    enum { n_fns = 2 };
    easing_fn fns_under_test[n_fns+1] = {
        easing_linear, easing_cubic, NULL
    };
    plan(3 * n_fns);
    long seed = time(NULL);
    note("srand48(%ld)\n", seed);
    srand48(seed);
    for (easing_fn *f = fns_under_test; *f; ++f) {
        float start, end, duration, t;
        start = drand48();
        end = drand48();
        duration = drand48();
        t = ease_float(start, end, 0., duration, *f);
        ok(fabs(t-start) < EPSILON, "f(0) = %f == %f = start", t, start);
        t = ease_float(start, end, duration, duration, *f);
        ok(fabs(t-end) < EPSILON, "f(duration) = %f == %f = end", t, end);
        t = ease_float(start, end, duration/2, duration, *f);
        ok(fabs(t-start) < fabs(end-start)+EPSILON, "%f is in [%f,%f]", t, start, end);
    }
    done_testing();
}
#endif
