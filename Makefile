CC := clang -std=c99

CFLAGS := -pedantic -Werror -Wall -Wextra -Wshadow -Wundef -Wunreachable-code \
          -Wold-style-definition -Wmissing-declarations -Wmissing-prototypes  \
          -Wstrict-prototypes -Wnested-externs -Wwrite-strings -Wwrite-strings\
          -Winit-self -Wwrite-strings -Wbad-function-cast -Wstrict-overflow=3 \
          -Wpointer-arith -Wno-missing-field-initializers -Wno-dangling-else  \
          -Wno-unused-parameter -Wno-logical-op-parentheses -Wno-switch

GPERF := gperf
RM := rm

COMMON := src/utils.o src/iterate.o src/lexer.o src/parser.o


clint: $(COMMON) src/cli.c
	$(CC) $(CFLAGS) $^ -o $@

run-test: $(COMMON) test/*
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
