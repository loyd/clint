/*!
 * @mainpage
 * @brief The main header file.
 */

#ifndef __CLINT_H__
#define __CLINT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tokens.h"
#include "tree.h"


typedef struct {
    char *name;
    char *data;
    char **lines;  //!< 1-indexed
    int nlines;
    tree_t tree;
    token_t *tokens;
} file_t;


/*!
 * @name Memory management
 * Never return `NULL`.
 */
//!@{
extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);
//!@}


/*!
 * @name Logging
 */
//!@{
extern void resume_warnings(void);
extern void pause_warnings(void);

extern void *warn_at(file_t *file, int line, int column, const char *fmt, ...)
  __attribute__((format(printf, 4, 5)));
//!@}


/*!
 * @name Lexer
 */
//!@{
extern void init_lexer(file_t *file);
extern void pull_token(token_t *token);
extern const char *stringify_token(token_t *token);
//!@}

/*!
 * @name Parser
 */
//!@{
extern void parse(file_t *file);
//!@}

#endif  // __CLINT_H__
