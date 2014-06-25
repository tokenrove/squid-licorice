.PHONY: clean all test check check-syntax

CONFIGURATION	?= DEBUG
CC		?= gcc
CLANG		?= clang

PACKAGES=sdl2 gl glu glew
CFLAGS_WARN=-Wall -Wextra -Wshadow -Winit-self -Wwrite-strings -Wswitch -Wswitch-default -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wformat-security -fstrict-aliasing -Wstrict-aliasing
CFLAGS_INCLUDE=-Ivendor -Iobj `pkg-config --cflags $(PACKAGES)`
CFLAGS_BASE=-fms-extensions -std=gnu11
CFLAGS_RELEASE=-O3
CFLAGS_DEBUG=-g -pg
CFLAGS=$(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE) $(CFLAGS_$(CONFIGURATION))
LDFLAGS_DEBUG=
LDFLAGS_RELEASE=-fwhole-program
LDFLAGS_LIBS=`pkg-config --libs $(PACKAGES)` -lpnglite -lz -lm
LDFLAGS=$(LDFLAGS_LIBS) $(LDFLAGS_$(CONFIGURATION))
VPATH=src
ENGINE_SRC=timer.c texture.c shader.c tilemap.c sprite.c text.c video.c gl.c strand.c input.c camera.c easing.c alloc_bitmap.c log.c
GAME_SRC=layer.c actor.c physics.c stage.c level.c game.c osd.c main.c
SRC=$(ENGINE_SRC) $(GAME_SRC)
OBJECTS=$(addprefix obj/, $(SRC:.c=.o))
DEPS=$(OBJECTS:%.o=%.d)
CLEAN=$(OBJECTS) $(DEPS) obj/squid $(TESTS)

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

obj/%.o: src/%.c | obj/
	$(CC) -MD -c $(CFLAGS) -o $@ $<
obj/%.vert.i: src/%.vert
	xxd -i < $< > $@
obj/%.frag.i: src/%.frag
	xxd -i < $< > $@

vendor/libtap/libtap.a:
	$(MAKE) -C vendor/libtap
vendor/glew/lib/libGLEW.a:
	$(MAKE) -C vendor/glew SYSTEM=linux-osmesa extensions all

TESTS=t/strand.t t/easing.t t/shader.t t/tilemap.t t/actor.t t/camera.t t/input.t t/layer.t t/physics.t t/sprite.t t/text.t t/texture.t
CFLAGS_TEST=-fprofile-arcs -ftest-coverage -fstack-usage -Ivendor/glew/include $(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE)
LDFLAGS_TEST=-Lvendor/glew/lib vendor/glew/lib/libGLEW.a $(LDFLAGS_LIBS) -Lvendor/libtap -ltap -lOSMesa -lgcov

$(TESTS): | vendor/libtap/libtap.a vendor/glew/lib/libGLEW.a t/

t/easing.t: src/easing.c
	$(CC) -DUNIT_TEST_EASING $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/strand.t: src/strand.c
	$(CC) -DUNIT_TEST_STRAND $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/alloc_bitmap.t: src/alloc_bitmap.c src/log.c
	$(CC) -DUNIT_TEST_ALLOC_BITMAP $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/shader.t: src/shader.c src/log.c src/test_video.c src/gl.c
	$(CC) -DUNIT_TEST_SHADER $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/actor.t: src/actor.c src/input.c src/log.c src/alloc_bitmap.c src/physics.c src/sprite.c src/texture.c src/camera.c src/test_video.c src/gl.c src/shader.c
	$(CC) -DUNIT_TEST_ACTOR $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/camera.t: src/camera.c src/test_video.c src/gl.c src/log.c
	$(CC) -DUNIT_TEST_CAMERA $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/input.t: src/input.c src/log.c
	$(CC) -DUNIT_TEST_INPUT $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/texture.t: src/texture.c src/log.c
	$(CC) -DUNIT_TEST_TEXTURE $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/layer.t: src/layer.c src/tilemap.c src/test_video.c src/gl.c src/camera.c src/log.c src/texture.c src/shader.c
	$(CC) -DUNIT_TEST_LAYER $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/physics.t: src/physics.c src/alloc_bitmap.c src/log.c
	$(CC) -DUNIT_TEST_PHYSICS $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/tilemap.t: src/tilemap.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
	$(CC) -DUNIT_TEST_TILEMAP $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/sprite.t: src/sprite.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
	$(CC) -DUNIT_TEST_SPRITE $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)
t/text.t: src/text.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
	$(CC) -DUNIT_TEST_TEXT $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

test: check check-syntax

check: $(TESTS)
	MESA_DEBUG=1 prove

# XXX use scan-build make instead?
check-syntax: $(SRC)
	$(CC) $(CFLAGS) -pedantic -Werror -fsyntax-only $^
	$(CLANG) --analyze -Weverything -Wextra -Werror -pedantic $(CFLAGS_INCLUDE) $^

clean:
	$(RM) $(CLEAN)

-include $(DEPS)
