#ifndef CLINT_H
#define CLINT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tokens.h"


enum token_e {
#define XX(kind, word) kind,
  TOK_MAP(XX)
#undef XX
};


typedef struct {
  enum token_e kind;

  struct {
    int line;    // 1-indexed
    int column;  // 1-indexed
  } start, end;
} token_t;


typedef struct {
  char *name;
  char *data;
  char **lines;  // 1-indexed
  int nlines;
} file_t;


/* Memory management.
 */
extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);


/* Logging.
 */
extern void *warn_at(file_t *file, int line, int column, const char *fmt, ...)
  __attribute__((format(printf, 4, 5)));


/* Lexer.
 */
extern void lex_init(file_t *file);
extern bool lex_pull(token_t *token);
extern const char *lex_to_str(token_t *token);

#endif
