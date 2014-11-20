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
    char *start;    //!< Place within `g_data`.
    bool dangling;  //!< w/ backslash + newline.
} line_t;


/*!
 * Global state.
 */
//!@{
extern char     *g_filename;    //!< Name of the current file.
extern char     *g_data;        //!< Content of the current file.
extern line_t   *g_lines;       //!< Pointers to starts of line.
extern tree_t    g_tree;        //!< Tree of the current file.
extern token_t  *g_tokens;      //!< 1-indexed consumed tokens.
//!@}


extern void load_file(const char *filename);
extern void reset_state(void);

extern void dispose_tree(tree_t tree);
extern char *stringify_tree(void);
extern char *stringify_tokens(void);


/*!
 * @name Memory management
 * Never return `NULL`.
 */
//!@{
extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);
//!@}


/*!
 * @name Vector interface.
 */
//!@{
#define new_vec(type, init_capacity) new_vec(sizeof(type), (init_capacity));
#define vec_len(vec) (((size_t *)(void *)(vec))[-1])
#define vec_push(vec, elem)                                                   \
    *(vec_expand_if_need((void **)&(vec)), &vec[vec_len(vec)++]) = elem

extern void *(new_vec)(size_t elem_sz, size_t init_capacity);
extern void vec_expand_if_need(void **vec_ptr);
extern void free_vec(void *vec);
//!@}


/*!
 * @name Logging
 */
//!@{
extern void resume_warnings(void);
extern void pause_warnings(void);

extern void *warn_at(unsigned line, unsigned column, const char *fmt, ...)
  __attribute__((format(printf, 3, 4)));
//!@}


/*!
 * @name Lexer
 */
//!@{
extern void init_lexer(void);
extern void pull_token(token_t *token);
//!@}


/*!
 * @name Parser
 */
//!@{
extern void init_parser(void);
extern void parse(void);
//!@}

#endif  // __CLINT_H__
