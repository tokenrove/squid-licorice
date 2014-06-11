.PHONY: clean all test check check-syntax

CLANG=clang
PACKAGES=sdl2 gl glu glew
CFLAGS=-Wall -Wextra -Wshadow -Winit-self -Wwrite-strings -Wswitch -Wswitch-default -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wformat-security -fstrict-aliasing -Wstrict-aliasing -fms-extensions -std=gnu11 -g -O3 -Ivendor -Iobj `pkg-config --cflags $(PACKAGES)`
LDFLAGS=-fwhole-program `pkg-config --libs $(PACKAGES)` -lpnglite -lz -lm
VPATH=src
ENGINE_SRC=timer.c texture.c shader.c tilemap.c sprite.c text.c video.c strand.c input.c camera.c easing.c alloc_bitmap.c log.c
GAME_SRC=layer.c actor.c physics.c stage.c level.c game.c osd.c main.c
SRC=$(ENGINE_SRC) $(GAME_SRC)
OBJECTS=$(addprefix obj/, $(SRC:.c=.o))
DEPS=$(OBJECTS:%.o=%.d)
CLEAN=$(OBJECTS) $(DEPS) obj/squid

all: obj/squid

src/tilemap.c: obj/tilemap.vert.i obj/tilemap.frag.i
src/text.c: obj/text.vert.i obj/text.frag.i
src/sprite.c: obj/sprite.vert.i obj/sprite.frag.i

obj/squid: $(OBJECTS) | obj
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj:
	mkdir -p obj

obj/%.o: src/%.c
	$(CC) -MD -c $(CFLAGS) -o $@ $<
obj/%.vert.i: src/%.vert
	xxd -i < $< > $@
obj/%.frag.i: src/%.frag
	xxd -i < $< > $@

test: check check-syntax

check: $(TESTS)
	prove

# XXX use scan-build make instead?
check-syntax: $(SRC)
	$(CLANG) --analyze -Weverything -Wextra -Werror -pedantic $^
	$(CC) $(CFLAGS) -pedantic -Werror -fsyntax-only $^

clean:
	$(RM) $(CLEAN)

-include $(DEPS)
