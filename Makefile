.PHONY: clean all test check check-syntax

PACKAGES=sdl2 gl glu glew
CFLAGS=-Wall -Wextra -fstrict-aliasing -Wstrict-aliasing -fms-extensions -std=gnu11 -g -O3 -Ivendor `pkg-config --cflags $(PACKAGES)`
LDFLAGS=`pkg-config --libs $(PACKAGES)`
VPATH=src
SRC=
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

check-syntax: $(SRC)
	$(CLANG) --analyze -Wall -Wextra -Werror -pedantic $^
	$(CC) $(CFLAGS) -pedantic -Werror -fsyntax-only $^

clean:
	$(RM) $(CLEAN)

-include $(DEPS)
