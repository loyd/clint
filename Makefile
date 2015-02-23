CC := clang -std=c99

CFLAGS := -pedantic -Werror -Wall -Wextra -Wshadow -Wundef -Wunreachable-code \
          -Winit-self -Wbad-function-cast -Wstrict-overflow=3 -Wpointer-arith \
          -Wold-style-definition -Wmissing-declarations -Wmissing-prototypes  \
          -Wstrict-prototypes -Wnested-externs -Wwrite-strings -Wwrite-strings\
          -Wno-missing-field-initializers -Wno-sign-compare -Wno-parentheses  \
          -Wno-logical-op-parentheses -Wno-unused-parameter -Wno-unused-value \
          -Wno-dangling-else

CFLAGS += -DVERSION=\"$(shell cat VERSION)\"
CFLAGS += -D_XOPEN_SOURCE=500
CFLAGS += -iquotesrc
CFLAGS += -Ideps/json-parser

RM := rm

PROGOBJS := $(wildcard src/*.c rules/*.c) deps/json-parser/json.c
TESTOBJS := $(filter-out src/cli.c,$(PROGOBJS) $(wildcard test/*.c))


clint: $(PROGOBJS:.c=.o)
	$(CC) -lm $(CFLAGS) $^ -o $@

run-test: $(TESTOBJS:.c=.o)
	$(CC) -lm $(CFLAGS) $^ -o $@
	./$@

%.o: %.c */*.h
	$(CC) -c $(CFLAGS) $< -o $@

test/test-parser.o: test/test-parser.txt

.PHONY: lint clean
lint: clint
	./clint -s src rules test

clean:
	$(RM) -f */*.o */*/*.o clint run-test clint.exe run-test.exe
