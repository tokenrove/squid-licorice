.PHONY: clean all test check check-syntax coverage

CONFIGURATION	?= DEBUG
CC		?= gcc
CLANG		?= clang

.SHELLFLAGS	:= -ec
PACKAGES	:= sdl2 gl glu glew
CFLAGS_WARN	:= -Wall -Wextra -Wshadow -Winit-self -Wwrite-strings -Wswitch -Wswitch-default -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wformat-security -fstrict-aliasing -Wstrict-aliasing
CFLAGS_INCLUDE	:= -Ivendor -Iobj `pkg-config --cflags $(PACKAGES)`
CFLAGS_BASE	:= -fms-extensions -std=gnu11
CFLAGS_RELEASE	:= -O3
CFLAGS_DEBUG	:= -g -pg -DDEBUG
CFLAGS		 = $(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE) $(CFLAGS_$(CONFIGURATION))
LDFLAGS_DEBUG	:=
LDFLAGS_RELEASE :=-fwhole-program
LDFLAGS_LIBS	:=`pkg-config --libs $(PACKAGES)` -lpnglite -lz -lm
LDFLAGS		 = $(LDFLAGS_LIBS) $(LDFLAGS_$(CONFIGURATION))
VPATH		:= src
ENGINE_SRC	:= timer.c texture.c shader.c tilemap.c sprite.c text.c video.c gl.c strand.c input.c camera.c easing.c alloc_bitmap.c log.c
GAME_SRC	:= layer.c actor.c physics.c stage.c level.c game.c osd.c main.c
SRC		:= $(ENGINE_SRC) $(GAME_SRC)
OBJECTS		:= $(addprefix obj/, $(SRC:.c=.o))
DEPS		:= $(OBJECTS:%.o=%.d)
CLEAN		 = $(OBJECTS) $(DEPS) obj/squid $(TESTS)

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

TESTS       := t/actor.t t/alloc_bitmap.t t/camera.t t/easing.t t/input.t t/layer.t t/physics.t t/shader.t t/sprite.t t/strand.t t/text.t t/texture.t t/tilemap.t
CFLAGS_TEST  = -fprofile-arcs -ftest-coverage -fstack-usage -Ivendor/glew/include $(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE) -DDEBUG -DTESTING
LDFLAGS_TEST = -Lvendor/glew/lib vendor/glew/lib/libGLEW.a $(LDFLAGS_LIBS) -Lvendor/libtap -ltap -lOSMesa -lgcov

$(TESTS): | vendor/libtap/libtap.a vendor/glew/lib/libGLEW.a t/
	$(CC) -DUNIT_TEST_$(shell echo $(basename $(notdir $@)) | tr '[:lower:]' '[:upper:]') $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/actor.t: src/actor.c src/input.c src/log.c src/alloc_bitmap.c src/physics.c src/sprite.c src/texture.c src/camera.c src/test_video.c src/gl.c src/shader.c
t/alloc_bitmap.t: src/alloc_bitmap.c src/log.c
t/camera.t: src/camera.c src/test_video.c src/gl.c src/log.c
t/easing.t: src/easing.c
t/input.t: src/input.c src/log.c
t/layer.t: src/layer.c src/tilemap.c src/test_video.c src/gl.c src/camera.c src/log.c src/texture.c src/shader.c
t/physics.t: src/physics.c src/alloc_bitmap.c src/log.c
t/shader.t: src/shader.c src/log.c src/test_video.c src/gl.c
t/sprite.t: src/sprite.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
t/strand.t: src/strand.c
t/text.t: src/text.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
t/texture.t: src/texture.c src/log.c
t/tilemap.t: src/tilemap.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c

test: check check-syntax

check: $(TESTS)
	MESA_DEBUG=1 prove

coverage: check
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage-report

# XXX use scan-build make instead?
check-syntax: $(SRC)
	$(CC) $(CFLAGS) -pedantic -Werror -fsyntax-only $^
	$(CLANG) --analyze -Weverything -Wextra -Werror -pedantic $(CFLAGS_INCLUDE) $^

clean:
	$(RM) $(CLEAN)

-include $(DEPS)
