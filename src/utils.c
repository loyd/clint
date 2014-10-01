#include "clint.h"

#include <assert.h>
#include <stdlib.h>


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
