/*!
 * @mainpage
 * @brief The main header file.
 */

#ifndef __CLINT_H__
#define __CLINT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <json.h>

#include "tokens.h"
#include "tree.h"


typedef struct {
    char *start;        //!< Place within `g_data`.
    unsigned length;    //!< The length w/o line break.
    bool dangling;      //!< w/ backslash + newline.
} line_t;


typedef struct {
    bool stylistic;
    unsigned line;
    unsigned column;
    char *message;
} error_t;


/*!
 * Global state.
 */
//!@{
extern char *g_filename;        //!< Name of the current file.
extern char *g_data;            //!< Content of the current file.
extern line_t *g_lines;         //!< Pointers to starts of line.
extern tree_t g_tree;           //!< Tree of the current file.
extern bool g_cached;           //!< The tree is cached or not.
extern token_t *g_tokens;       //!< 1-indexed consumed tokens.
extern error_t *g_errors;       //!< Errors and warnings.
extern json_value *g_config;    //!< Root of the config file.
//!@}


/*!
 * @name Rules runner.
 */
//!@{
struct rule_s {
    const char *name;
    void (*configure)(void);
    void (*check)(void);
    json_value *config;
};

#define REGISTER_RULE(name, configure, check)                                 \
    struct rule_s name ## _rule = {#name, configure, check, NULL}

extern bool configure_rules(void);
extern void check_rules(void);

extern void cfg_fatal(const char *prop, const char *message);
extern json_type cfg_typeof(const char *prop);
extern bool cfg_boolean(const char *prop);
extern char *cfg_string(const char *prop);
extern unsigned cfg_natural(const char *prop);
extern char **cfg_strings(const char *prop);
//!@}

extern void reset_state(void);
extern void dispose_tree(tree_t tree);


extern const char *stringify_type(enum type_e type);
extern const char *stringify_kind(enum token_e kind);

extern char *stringify_tree(void);
extern char *stringify_tokens(void);


/*!
 * @name Memory management.
 * Never return `NULL`.
 */
//!@{
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t num, size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern char *xstrdup(const char *src);
//!@}


/*!
 * @name Vector interface.
 */
//!@{
#define new_vec(type, init_capacity) new_vec(sizeof(type), (init_capacity));
#define vec_len(vec) (((size_t *)(void *)(vec))[-1])
#define vec_push(vec, elem)                                                   \
    (vec_expand_if_need((void **)&(vec)),                                     \
     vec[vec_len(vec)] = (elem),                                              \
     ++vec_len(vec))
#define vec_pop(vec) vec[--vec_len(vec)]

extern void *(new_vec)(size_t elem_sz, size_t init_capacity);
extern void vec_expand_if_need(void **vec_ptr);
extern void free_vec(void *vec);
//!@}


/*!
 * @name Logging.
 */
//!@{
enum log_mode_e {
    LOG_SORTED  = 1 << 0,
    LOG_SILENCE = 1 << 1,
    LOG_VERBOSE = 1 << 2,
    LOG_SHORTLY = 1 << 3,
    LOG_COLOR   = 1 << 4
};

extern enum log_mode_e g_log_mode;
extern unsigned g_log_limit;


extern void add_log(bool stylistic, unsigned line, unsigned column,
    const char *fmt, ...) __attribute__((format(printf, 4, 5)));

#define add_warn(...)          add_log(true, __VA_ARGS__)
#define add_error(...)         add_log(false, __VA_ARGS__)
#define add_warn_at(loc, ...)  add_warn((loc).line, (loc).column, __VA_ARGS__)
#define add_error_at(loc, ...) add_error((loc).line, (loc).column, __VA_ARGS__)


extern void print_errors_in_order(void);
//!@}


/*!
 * @name Lexer.
 */
//!@{
extern void init_lexer(void);
extern void pull_token(token_t *token);
extern void tokenize(void);
//!@}


/*!
 * @name Parser.
 */
//!@{
extern void init_parser(void);
extern void parse(void);
//!@}


/*!
 * @name Tree-walk.
 */
//!@{
typedef void (*visitor_t)(tree_t tree);

#define iterate_by_type(type, cb) iterate_by_type(type, (visitor_t)cb)
extern void (iterate_by_type)(enum type_e type, visitor_t cb);
//!@}

#endif  // __CLINT_H__
