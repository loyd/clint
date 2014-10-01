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


clint: src/*.[ch] src/kw.c.part src/pp.c.part
	$(CC) $(CFLAGS) src/*.c -o $@

src/%.c.part: src/%.gperf
	$(GPERF) -tlTCE -j1 -K word -H $*_hash -N $*_check $< > $@

.PHONY: clean full-clean
clean:
	$(RM) -rf clint

full-clean: clean
	$(RM) src/*.c.part
