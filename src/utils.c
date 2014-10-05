#include "clint.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void *xmalloc(size_t size) {
  assert(size > 0);
  void *ptr = malloc(size);
  if (!ptr) abort();
  return ptr;
}


void *xrealloc(void *ptr, size_t size) {
  assert(size > 0);
  ptr = realloc(ptr, size);
  if (!ptr) abort();
  return ptr;
}


static inline int count_signs(int num) {
  assert(num >= 0);
  int res = 0;

  while (num) {
    ++res;
    num /= 10;
  }

  return res;
}


static inline int get_line_len(file_t *file, int line) {
  assert(file);
  assert(0 < line && line <= file->nlines);

  if (line == file->nlines) {
    char *nl = strchr(file->lines[line], '\n');
    return nl ? nl - file->lines[line] : strlen(file->lines[line]);
  }

  return file->lines[line+1] - file->lines[line] - 1;
}


#define MESSAGE_STYLE   "\x1b[1m"
#define FILENAME_STYLE  "\x1b[32;1m"
#define NORMAL_STYLE    "\x1b[0m"

void *warn_at(file_t *file, int line, int column, const char *format, ...) {
  assert(file && format);
  assert(0 < line && line <= file->nlines);

  int line_len = get_line_len(file, line);
  assert(0 < column && column <= line_len + 1);

  va_list arg;
  int line_from = line-2;
  int line_to = line+2;

  if (line_from < 1) line_from = 1;
  if (line_to > file->nlines) line_to = file->nlines;

  int line_width = count_signs(line_to);

  char pointer[column + line_width + 6];
  memset(pointer, '-', sizeof(pointer) - 2);
  pointer[sizeof(pointer) - 2] = '^';
  pointer[sizeof(pointer) - 1] = '\0';

  fprintf(stderr, MESSAGE_STYLE);
  va_start(arg, format);
  vfprintf(stderr, format, arg);
  va_end(arg);

  if (file->name)
    fprintf(stderr, "%s%s", NORMAL_STYLE " at " FILENAME_STYLE, file->name);

  fprintf(stderr, NORMAL_STYLE ":\n");

  for (int i = line_from; i <= line_to; ++i) {
    fprintf(stderr, "  %*d | %.*s\n", line_width, i, get_line_len(file, i),
                                      file->lines[i]);
    if (i == line)
      fprintf(stderr, "%s\n", pointer);
  }

  fprintf(stderr, "\n");

  return NULL;
}
