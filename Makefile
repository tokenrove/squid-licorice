.PHONY: clean all test check check-syntax coverage assets

CONFIGURATION	?= DEBUG
CC		?= gcc
CLANG		?= clang

.SHELLFLAGS	:= -ec
PACKAGES	:= sdl2 gl glu glew
CFLAGS_WARN	:= -Wall -Wextra -Wshadow -Winit-self -Wwrite-strings -Wswitch -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wformat-security -fstrict-aliasing -Wstrict-aliasing
CFLAGS_INCLUDE	:= -Ivendor -Iobj `pkg-config --cflags $(PACKAGES)`
CFLAGS_BASE	:= -fms-extensions -std=gnu11
CFLAGS_RELEASE	:= -O3
CFLAGS_DEBUG	:= -g -pg -DDEBUG
CFLAGS		 = $(CFLAGS_WARN) -Wswitch-default $(CFLAGS_BASE) $(CFLAGS_INCLUDE) $(CFLAGS_$(CONFIGURATION))
LDFLAGS_DEBUG	:=
LDFLAGS_RELEASE :=-fwhole-program
LDFLAGS_LIBS	:=`pkg-config --libs $(PACKAGES)` -lpnglite -lz -lm
LDFLAGS		 = $(LDFLAGS_LIBS) $(LDFLAGS_$(CONFIGURATION))
VPATH		:= src
ENGINE_SRC	:= timer.c texture.c shader.c tilemap.c sprite.c text.c video.c gl.c strand.c input.c camera.c easing.c alloc_bitmap.c log.c utf8.c msg.c draw.c point_sprite.c
GAME_SRC	:= layer.c actor.c physics.c stage.c level.c game.c osd.c main.c player.c enemy.c projectile.c
SRC		:= $(ENGINE_SRC) $(GAME_SRC)
OBJECTS		:= $(addprefix obj/, $(SRC:.c=.o))
DEPS		:= $(OBJECTS:%.o=%.d)
CLEAN		 = $(OBJECTS) $(DEPS) obj/squid $(TESTS)

all: obj/squid assets check

obj/:
	mkdir -p obj/
t/:
	mkdir -p t/
data/:
	mkdir -p data/

#### OBJ/SQUID: MAIN EXECUTABLE

src/tilemap.c: obj/tilemap.vert.i obj/tilemap.frag.i
src/text.c: obj/text.vert.i obj/text.frag.i
src/sprite.c: obj/sprite.vert.i obj/sprite.frag.i
src/draw.c: obj/draw.vert.i obj/draw.frag.i
src/point_sprite.c: obj/point_sprite.vert.i obj/point_sprite.frag.i

obj/squid: $(OBJECTS) | obj/
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.c | obj/
	$(CC) -MD -c $(CFLAGS) -o $@ $<
obj/%.vert.i: src/%.vert
	xxd -i < $< > $@
obj/%.frag.i: src/%.frag
	xxd -i < $< > $@

#### LIBRARIES

vendor/libtap/libtap.a:
	$(MAKE) -C vendor/libtap
vendor/glew/lib/libGLEW.a:
	$(MAKE) -C vendor/glew SYSTEM=linux-osmesa extensions all

#### ASSETS

STATIC_TEXTURES := data/projectiles.png data/sprites.png
SLABS := slice1 slice2 slice3
FONTS := osd

assets: data/ $(STATIC_TEXTURES) $(addprefix data/,$(addsuffix .map,$(SLABS)) $(addsuffix .map,$(SLABS))) $(addprefix data/,$(addsuffix .fnt,$(FONTS)))

data/%.png: art/static/%.png
	pngcrush -c 6 -brute $< $@

data/%.map data/%.png: art/slab/%.png
	python mortimer2.py $< $(basename $@).map $(basename $@).png

data/%.fnt data/%.png: art/font/%.fnt art/font/%.png
	ln $^ data/

#### TESTS

## NB: Because strand.t is very sensative to stack usage, do not
## compile it with -pg without adjusting the sizes in the test.
## Probably, the test itself should be more introspective to figure
## these things out.
TESTS       := t/actor.t t/alloc_bitmap.t t/camera.t t/easing.t t/input.t t/layer.t t/msg.t t/physics.t t/point_sprite.t t/projectile.t t/shader.t t/sprite.t t/strand.t t/text.t t/texture.t t/tilemap.t t/utf8.t
CFLAGS_TEST  = -O3 -fprofile-arcs -ftest-coverage -fstack-usage -g -Ivendor/glew/include $(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE) -DDEBUG -DTESTING
LDFLAGS_TEST = -Lvendor/glew/lib vendor/glew/lib/libGLEW.a $(LDFLAGS_LIBS) -Lvendor/libtap -ltap -lOSMesa -lgcov

$(TESTS): | vendor/libtap/libtap.a vendor/glew/lib/libGLEW.a t/
	$(CC) -DUNIT_TEST_$(shell echo $(basename $(notdir $@)) | tr '[:lower:]' '[:upper:]') $(CFLAGS_TEST) -g -o $@ $^ $(LDFLAGS_TEST)

t/actor.t: src/actor.c src/input.c src/log.c src/alloc_bitmap.c src/physics.c src/sprite.c src/texture.c src/camera.c src/test_video.c src/gl.c src/shader.c src/msg.c
t/alloc_bitmap.t: src/alloc_bitmap.c src/log.c
t/camera.t: src/camera.c src/test_video.c src/gl.c src/log.c
t/easing.t: src/easing.c
t/input.t: src/input.c src/log.c
t/layer.t: src/layer.c src/tilemap.c src/test_video.c src/gl.c src/camera.c src/log.c src/texture.c src/shader.c
t/msg.t: src/msg.c
t/physics.t: src/physics.c src/alloc_bitmap.c src/log.c src/msg.c
t/point_sprite.t: src/point_sprite.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
t/projectile.t: src/projectile.c src/point_sprite.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c src/msg.c src/physics.c src/alloc_bitmap.c
t/shader.t: src/shader.c src/log.c src/test_video.c src/gl.c
t/sprite.t: src/sprite.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
t/strand.t: src/strand.c
t/text.t: src/text.c src/utf8.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
t/texture.t: src/texture.c src/log.c src/test_video.c src/gl.c
t/tilemap.t: src/tilemap.c src/texture.c src/shader.c src/log.c src/camera.c src/test_video.c src/gl.c
t/utf8.t: src/utf8.c src/log.c


CFLAGS_PROFILE  = -O3 -Ivendor/glew/include $(CFLAGS_WARN) $(CFLAGS_BASE) $(CFLAGS_INCLUDE)
LDFLAGS_PROFILE = -Lvendor/glew/lib vendor/glew/lib/libGLEW.a $(LDFLAGS_LIBS) -Lvendor/libtap -ltap -lOSMesa
obj/alloc_bitmap.profiling: src/alloc_bitmap.c src/log.c
	$(CC) -DPROFILE_$(shell echo $(basename $(notdir $@)) | tr '[:lower:]' '[:upper:]') $(CFLAGS_PROFILE) -g -o $@ $^ $(LDFLAGS_PROFILE)

test: check check-syntax

PROVE ?= MESA_DEBUG=1 LD_LIBRARY_PATH=vendor/glew/lib prove

check: $(TESTS)
	$(PROVE)

check-with-valgrind: $(TESTS)
	$(PROVE) -e 'valgrind --error-exitcode=1'

#### COVERAGE

# This is a huge pain in the ass
LCOVFLAGS := --rc lcov_branch_coverage=1
coverage: clean
	$(RM) -r coverage-report *.lcov-info
	lcov $(LCOVFLAGS) -d . --zerocounters
	for i in $(TESTS); do \
	  $(MAKE) $$i && ./$$i ; lcov $(LCOVFLAGS) -c -d . -o $$(basename $$i).lcov-info ; \
	done
	lcov $(LCOVFLAGS) $$(for i in *.lcov-info; do echo -a $$i; done) -o total.lcov-info
	genhtml $(LCOVFLAGS) total.lcov-info --output-directory coverage-report

# XXX use scan-build make instead?
check-syntax: $(SRC)
	$(CC) $(CFLAGS) -pedantic -Werror -fsyntax-only $^
	$(CLANG) --analyze -Weverything -Wextra -Werror -pedantic $(CFLAGS_INCLUDE) $^

#### CLEAN AND DEPS

clean:
	$(RM) $(CLEAN)

-include $(DEPS)
