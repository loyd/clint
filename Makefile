CC := clang -std=c99

CFLAGS := -pedantic -Werror -Wall -Wextra -Wswitch-default -Wswitch-enum      \
          -Wshadow -Wundef -Wpointer-arith -Wcast-align -Winit-self           \
          -Wstrict-overflow=3 -Wlogical-op -Wwrite-strings -Wnested-externs   \
          -Wbad-function-cast -Wold-style-definition -Wunreachable-code       \
          -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations     \
          -Wno-logical-op-parentheses -Wno-unknown-warning-option             \
          -Wno-missing-field-initializers

GPERF := gperf
RM := rm

COMMON := src/utils.o src/lex.o


clint: $(COMMON) src/cli.c
	$(CC) $(CFLAGS) $^ -o $@

run-test: $(COMMON) test/*.[ch]
	$(CC) -iquotesrc $(COMMON) $(wildcard test/*.c) -o $@
	./$@

src/%.o: src/%.c src/*.h
	$(CC) -c $(CFLAGS) $< -o $@

src/lex.o: src/kw.c.part src/pp.c.part

src/%.c.part: src/%.gperf
	$(GPERF) -tlTCE -j1 -K word -H $*_hash -N $*_check $< > $@

.PHONY: clean
clean:
	$(RM) -f $(COMMON) clint run-test
