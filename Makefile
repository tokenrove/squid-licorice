.PHONY: clean all test check check-syntax

CLANG=clang
PACKAGES=sdl2 gl glu glew
CFLAGS=-Wall -Wextra -Wshadow -Winit-self -Wwrite-strings -Wswitch-enum -Wswitch-default -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wdouble-promotion -Wformat-security -fstrict-aliasing -Wstrict-aliasing -fms-extensions -std=gnu11 -g -O3 -Ivendor `pkg-config --cflags $(PACKAGES)`
LDFLAGS=-fwhole-program `pkg-config --libs $(PACKAGES)` -lpnglite -lz
VPATH=src
SRC=tilemap.c shader.c main.c
OBJECTS=$(addprefix obj/, $(SRC:.c=.o))
DEPS=$(OBJECTS:%.o=%.d)
CLEAN=$(OBJECTS) $(DEPS) obj/squid

obj/squid: $(OBJECTS) | obj
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj:
	mkdir -p obj

obj/%.o: src/%.c
	$(CC) -MD -c $(CFLAGS) -o $@ $<

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
