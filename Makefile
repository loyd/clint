CC := clang -std=c99

CFLAGS := -pedantic -Werror -Wall -Wextra -Wshadow -Wundef -Wunreachable-code \
          -Winit-self -Wbad-function-cast -Wstrict-overflow=3 -Wpointer-arith \
          -Wold-style-definition -Wmissing-declarations -Wmissing-prototypes  \
          -Wstrict-prototypes -Wnested-externs -Wwrite-strings -Wwrite-strings\
          -Wno-unused-parameter -Wno-logical-op-parentheses -Wno-dangling-else\
          -Wno-switch

RM := rm

COMMON := src/utils.o src/iterate.o src/lexer.o src/parser.o


clint: $(COMMON) src/cli.c
	$(CC) $(CFLAGS) $^ -o $@

run-test: $(COMMON) test/*
	$(CC) -iquotesrc $(COMMON) $(wildcard test/*.c) -o $@
	./$@

src/%.o: src/%.c src/*.h
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) -f src/*.o clint run-test
