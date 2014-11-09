CC := clang -std=c99

CFLAGS := -pedantic -Werror -Wall -Wextra -Wshadow -Wundef -Wunreachable-code \
          -Winit-self -Wstrict-overflow=3 -Wlogical-op -Wmissing-declarations \
          -Wmissing-prototypes -Wold-style-definition -Wstrict-prototypes     \
          -Wpointer-arith -Wnested-externs -Wbad-function-cast -Wwrite-strings\
          -Wno-switch -Wno-logical-op-parentheses -Wno-unknown-warning-option \
          -Wno-missing-field-initializers -Wno-dangling-else

GPERF := gperf
RM := rm

COMMON := src/utils.o src/lexer.o src/parser.o


clint: $(COMMON) src/cli.c
	$(CC) $(CFLAGS) $^ -o $@

run-test: $(COMMON) test/*.[ch]
	$(CC) -iquotesrc $(COMMON) $(wildcard test/*.c) -o $@
	./$@

src/%.o: src/%.c src/*.h
	$(CC) -c $(CFLAGS) $< -o $@

src/lexer.o: src/kw.c.part src/pp.c.part

src/%.c.part: src/%.gperf
	$(GPERF) -tlTCE -j1 -K word -H $*_hash -N $*_check $< > $@

.PHONY: clean
clean:
	$(RM) -f src/*.o clint run-test
