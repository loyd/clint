CC := clang -std=c99

CFLAGS := -pedantic -Werror -Wall -Wextra -Wshadow -Wundef -Wunreachable-code \
          -Winit-self -Wbad-function-cast -Wstrict-overflow=3 -Wpointer-arith \
          -Wold-style-definition -Wmissing-declarations -Wmissing-prototypes  \
          -Wstrict-prototypes -Wnested-externs -Wwrite-strings -Wwrite-strings\
          -Wno-unused-parameter -Wno-missing-field-initializers

ifneq (,$(findstring clang,$(CC)))
	CFLAGS += -Wno-logical-op-parentheses -Wno-dangling-else
endif

ifneq (,$(findstring gcc,$(CC)))
	CFLAGS += -Wno-parentheses -Wno-sign-compare
endif

CFLAGS += -DVERSION=\"$(shell cat VERSION)\"
CFLAGS += -D_XOPEN_SOURCE=500
CFLAGS += -Ideps/json-parser
CFLAGS += -iquotesrc

RM := rm

COMMON := src/state.o src/utils.o src/iterate.o src/lexer.o src/parser.o


clint: $(COMMON) deps/json-parser/json.c rules/*.c src/rules.c src/cli.c
	$(CC) -lm $(CFLAGS) $^ -o $@

run-test: $(COMMON) test/*
	$(CC) $(CFLAGS) $(COMMON) $(wildcard test/*.c) -o $@
	./$@

src/%.o: src/%.c src/*.h
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) -f src/*.o clint run-test clint.exe run-test.exe
