#pragma once

static inline float maxf(const float a, const float b) { return a < b ? b : a; }

/* Per Hacker's Delight, 3-2 */
static inline size_t closest_power_of_2(size_t x)
{
    return 1 << ((8*sizeof (x)) - __builtin_clz(x-1));
}

