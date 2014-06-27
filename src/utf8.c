/* Very primitive UTF-8 decoding. */

#include "utf8.h"
#include "log.h"

uint32_t decode_utf8(const char **s)
{
    uint8_t c = **s;
    ++*s;
    if (c < 128)
        return c;
    c <<= 1;
    if(!(c&0x80)) goto invalid;
    int n;
    for (n = 0; c&0x80; ++n) c <<= 1;
    uint32_t u = c>>(1+n);
    while (n--) {
        c = **s;
        if (!c) goto invalid;
        ++*s;
        if (0x80 != (c&0xc0)) goto invalid;
        u = (u<<6) | (c&0x3f);
    }
    return u;
invalid:
    LOG_DEBUG("Encountered invalid UTF-8 encoded string; discarding.");
    return 0;
}


#ifdef UNIT_TEST_UTF8
#include "libtap/tap.h"
#include <stddef.h>
#include <string.h>

static void apply_fuzz_test_to(void (*fn)(void *, const char *), void *u)
{
    for (int i = 1000; i > 0; --i) {
        char buf[128];
        for (int j = 0; j < 128; ++j)
            buf[j] = (uint8_t)(rand() & 0xff);
        buf[127] = 0;
        fn(u, buf);
    }
}
static void test_low_level_utf8_decoding(void)
{
    note("Testing valid UTF-8");
    {
        const char *s = "\u201cMontréal, Québec\u201d";
        const uint32_t expected[] = {
            0x201c, 'M', 'o', 'n', 't', 'r', 0xe9, 'a', 'l', ',', ' ',
            'Q', 'u', 0xe9, 'b', 'e', 'c', 0x201d
        };
        for (const uint32_t *p = expected; *s; ++p) cmp_ok(decode_utf8(&s), "==", *p);
    }
    {
        note("Greek text");
        const char *s = "\316\272\341\275\271\317\203\316\274\316\265";
        const uint32_t expected[] = {  /* κόσμε */
            0x3ba,0x1f79,0x3c3,0x3bc,0x3b5
        };
        for (const uint32_t *p = expected; *s; ++p) cmp_ok(decode_utf8(&s), "==", *p);
    }
    {
        note("Hebrew text");
        /* ? דג סקרן שט בים מאוכזב ולפתע מצא לו חברה איך הקליטה */
        const char t[] = {
0x3f,0x20,0xd7,0x93,0xd7,0x92,0x20,0xd7,0xa1,0xd7,0xa7,0xd7,0xa8,0xd7,0x9f,0x20,
0xd7,0xa9,0xd7,0x98,0x20,0xd7,0x91,0xd7,0x99,0xd7,0x9d,0x20,0xd7,0x9e,0xd7,0x90,
0xd7,0x95,0xd7,0x9b,0xd7,0x96,0xd7,0x91,0x20,0xd7,0x95,0xd7,0x9c,0xd7,0xa4,0xd7,
0xaa,0xd7,0xa2,0x20,0xd7,0x9e,0xd7,0xa6,0xd7,0x90,0x20,0xd7,0x9c,0xd7,0x95,0x20,
0xd7,0x97,0xd7,0x91,0xd7,0xa8,0xd7,0x94,0x20,0xd7,0x90,0xd7,0x99,0xd7,0x9a,0x20,
0xd7,0x94,0xd7,0xa7,0xd7,0x9c,0xd7,0x99,0xd7,0x98,0xd7,0x94,0};
        const char *s = t;
        const uint32_t expected[] = {
63, 32, 1491, 1490, 32, 1505, 1511, 1512, 1503, 32, 1513, 1496, 32, 1489, 1497,
1501, 32, 1502, 1488, 1493, 1499, 1494, 1489, 32, 1493, 1500, 1508, 1514, 1506,
32, 1502, 1510, 1488, 32, 1500, 1493, 32, 1495, 1489, 1512, 1492, 32, 1488,
1497, 1498, 32, 1492, 1511, 1500, 1497, 1496, 1492
        };
        for (const uint32_t *p = expected; *s; ++p) cmp_ok(decode_utf8(&s), "==", *p);
    }

    note("Testing invalid UTF-8");
    {
        const char *t = "\xF0\xA4\xAD \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD\xA2 \xF0\xA4\xAD", *s = t;
        while (*s) {
            decode_utf8(&s);
        }
        cmp_ok((uint8_t)*(s-1), "==", 0xad, "s should be one past the last byte");
        cmp_ok((ptrdiff_t)(s-t), "==", strlen(t), "s (%p) should be right at %p", s, t+strlen(t));
    }

    note("Testing truncated UTF-8");
    {
        const char *t = "\xf0", *s = t;
        ok(0 == decode_utf8(&s));
        ok(0 == *s);
        cmp_ok((uint8_t)*(s-1), "==", 0xf0, "s should be one past the last byte");
        cmp_ok((intptr_t)s-1, "==", (intptr_t)t, "s (%p) should be one past the beginning (%p)", s, t);
    }

    lives_ok({
            log_quiet();
            void fn(void *_ __attribute__ ((unused)), const char *s) {
                while (*s) decode_utf8(&s);
            };
            apply_fuzz_test_to(fn, NULL);
            log_noisy();
        }, "Fuzz testing UTF-8 decoding");
}

int main(void)
{
    plan(82);
    long seed = time(NULL);
    srand(seed);
    note("Random seed: %d", seed);
    test_low_level_utf8_decoding();
    done_testing();
}
#endif
