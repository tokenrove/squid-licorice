.PHONY: clean all test check check-syntax

CLANG=clang
PACKAGES=sdl2 gl glu glew
CFLAGS_WARN=-Wall -Wextra -Wshadow -Winit-self -Wwrite-strings -Wswitch -Wswitch-default -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wformat-security -fstrict-aliasing -Wstrict-aliasing
CFLAGS_INCLUDE=-Ivendor -Iobj `pkg-config --cflags $(PACKAGES)`
CFLAGS_BASE=-fms-extensions -std=gnu11
CFLAGS_RELEASE=-g -O3
CFLAGS=$(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE) $(CFLAGS_RELEASE)
LDFLAGS_LIBS=`pkg-config --libs $(PACKAGES)` -lpnglite -lz -lm
LDFLAGS=-fwhole-program $(LDFLAGS_LIBS)
VPATH=src
ENGINE_SRC=timer.c texture.c shader.c tilemap.c sprite.c text.c video.c strand.c input.c camera.c easing.c alloc_bitmap.c log.c
GAME_SRC=layer.c actor.c physics.c stage.c level.c game.c osd.c main.c
SRC=$(ENGINE_SRC) $(GAME_SRC)
OBJECTS=$(addprefix obj/, $(SRC:.c=.o))
DEPS=$(OBJECTS:%.o=%.d)
CLEAN=$(OBJECTS) $(DEPS) obj/squid

all: obj/squid check

src/tilemap.c: obj/tilemap.vert.i obj/tilemap.frag.i
src/text.c: obj/text.vert.i obj/text.frag.i
src/sprite.c: obj/sprite.vert.i obj/sprite.frag.i

obj/squid: $(OBJECTS) | obj/
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj/:
	mkdir -p obj/
t/:
	mkdir -p t/

obj/%.o: src/%.c
	$(CC) -MD -c $(CFLAGS) -o $@ $<
obj/%.vert.i: src/%.vert
	xxd -i < $< > $@
obj/%.frag.i: src/%.frag
	xxd -i < $< > $@

vendor/libtap/libtap.a:
	$(MAKE) -C vendor/libtap

TESTS=t/strand.t t/easing.t t/shader.t t/tilemap.t t/actor.t t/camera.t t/input.t t/layer.t t/physics.t t/sprite.t t/text.t t/texture.t
CFLAGS_TEST=$(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE) -fprofile-arcs -ftest-coverage
LDFLAGS_TEST=$(LDFLAGS_LIBS) -Lvendor/libtap -ltap

t/easing.t: src/easing.c | t/
	$(CC) -DUNIT_TEST_EASING $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/strand.t: src/strand.c | t/
	$(CC) -DUNIT_TEST_STRAND $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/alloc_bitmap.t: src/alloc_bitmap.c src/log.c | t/
	$(CC) -DUNIT_TEST_ALLOC_BITMAP $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/shader.t: src/shader.c src/log.c src/video.c | t/
	$(CC) -DUNIT_TEST_SHADER $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/actor.t: src/actor.c src/input.c src/log.c src/alloc_bitmap.c src/physics.c src/sprite.c src/texture.c src/camera.c src/video.c src/shader.c | t/
	$(CC) -DUNIT_TEST_ACTOR $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/camera.t: src/camera.c src/video.c src/log.c | t/
	$(CC) -DUNIT_TEST_CAMERA $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/input.t: src/input.c src/log.c | t/
	$(CC) -DUNIT_TEST_INPUT $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/texture.t: src/texture.c src/log.c | t/
	$(CC) -DUNIT_TEST_TEXTURE $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/layer.t: src/layer.c src/tilemap.c src/video.c src/camera.c src/log.c src/texture.c src/shader.c | t/
	$(CC) -DUNIT_TEST_LAYER $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/physics.t: src/physics.c src/alloc_bitmap.c src/log.c | t/
	$(CC) -DUNIT_TEST_PHYSICS $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/tilemap.t: src/tilemap.c src/texture.c src/shader.c src/log.c src/camera.c src/video.c | t/
	$(CC) -DUNIT_TEST_TILEMAP $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/sprite.t: src/sprite.c src/texture.c src/shader.c src/log.c src/camera.c src/video.c | t/
	$(CC) -DUNIT_TEST_SPRITE $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/text.t: src/text.c src/texture.c src/shader.c src/log.c src/camera.c src/video.c | t/
	$(CC) -DUNIT_TEST_TEXT $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

test: check check-syntax

check: $(TESTS)
	prove

# XXX use scan-build make instead?
check-syntax: $(SRC)
	$(CLANG) --analyze -Weverything -Wextra -Werror -pedantic $(CFLAGS_INCLUDE) $^
	$(CC) $(CFLAGS) -pedantic -Werror -fsyntax-only $^

clean:
	$(RM) $(CLEAN)

-include $(DEPS)
