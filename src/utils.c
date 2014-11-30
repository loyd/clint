/*!
 * @brief Some auxiliary functions.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clint.h"


///////////////////////
// Memory managment. //
///////////////////////

void *xmalloc(size_t size)
{
    assert(size > 0);

    void *ptr = malloc(size);
    if (!ptr)
        abort();

    return ptr;
}


void *xcalloc(size_t num, size_t size)
{
    assert(num > 0);
    assert(size > 0);

    void *ptr = calloc(num, size);
    if (!ptr)
        abort();

    return ptr;
}


void *xrealloc(void *ptr, size_t size)
{
    assert(size > 0);

    if (!(ptr = realloc(ptr, size)))
        abort();

    return ptr;
}


char *xstrdup(const char *src)
{
    assert(src);

    char *dup = strdup(src);
    if (!dup)
        abort();

    return dup;
}


////////////////////////////
// Vector implementation. //
////////////////////////////

/*  ________________________________________________________
 * | elem_sz | capacity | length | v[0] | v[1] | v[2] | ...|
 * ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 * <---------- header ----------><--------- data ---------->
 */

#define VEC_HEADER_SIZE (sizeof(size_t) * 3)

void *(new_vec)(size_t elem_sz, size_t init_capacity)
{
    size_t *res = xmalloc(VEC_HEADER_SIZE + elem_sz * init_capacity);

    res[0] = elem_sz;
    res[1] = init_capacity;
    res[2] = 0;

    return res + 3;
}


void vec_expand_if_need(void **vec_ptr)
{
    assert(vec_ptr);
    size_t *vec = *((size_t **)vec_ptr);

    if (vec[-1] != vec[-2])
        return;

    size_t elem_sz = vec[-3];
    size_t capacity = vec[-2] * 2;
    size_t len = vec[-1];

    vec = xrealloc(vec - 3, VEC_HEADER_SIZE + elem_sz * capacity);
    vec[0] = elem_sz;
    vec[1] = capacity;
    vec[2] = len;

    *vec_ptr = vec + 3;
}


void free_vec(void *vec)
{
    if (vec)
        free((char *)vec - VEC_HEADER_SIZE);
}


//////////////
// Logging. //
//////////////

static inline unsigned count_signs(unsigned num)
{
    unsigned res = 0;

    do
    {
        ++res;
        num /= 10;
    }
    while (num);

    return res;
}


static inline unsigned get_line_len(unsigned line)
{
    assert(line < vec_len(g_lines));

    if (line+1 == vec_len(g_lines))
    {
        char *nl = strchr(g_lines[line].start, '\n');
        return nl ? nl - g_lines[line].start : strlen(g_lines[line].start);
    }

    return g_lines[line+1].start - g_lines[line].start - 1;
}


static enum log_mode_e log_mode = LOG_STYLE;

void set_log_mode(enum log_mode_e level)
{
    log_mode = level;
}


void add_log(bool style, unsigned line, unsigned column, const char *fmt, ...)
{
    va_list arg;
    int len;
    char *msg;

    if (!g_errors)
        g_errors = new_vec(error_t, 24);

    va_start(arg, fmt);
    len = vsnprintf(NULL, 0, fmt, arg);
    va_end(arg);

    if (len <= 0)
        return;

    msg = xmalloc(len + 1);
    va_start(arg, fmt);
    vsnprintf(msg, len + 1, fmt, arg);
    va_end(arg);

    vec_push(g_errors, ((error_t){style, line, column, msg}));
}


#define MESSAGE_STYLE   "\x1b[1m"
#define FILENAME_STYLE  "\x1b[32;1m"
#define NORMAL_STYLE    "\x1b[0m"

static void print_error(error_t *error)
{
    assert(error->line < vec_len(g_lines));
    unsigned line_len, line_from, line_to, line_width;

    line_len = get_line_len(error->line);
    assert(error->column <= line_len);

    line_from = error->line > 2 ? error->line - 2 : 0;
    line_to = error->line + 2;

    if (line_to >= vec_len(g_lines))
        line_to = vec_len(g_lines) - 1;

    line_width = count_signs(line_to + 1);

    char pointer[2 + line_width + 3 + error->column + 2];
    memset(pointer, '-', sizeof(pointer) - 2);
    pointer[sizeof(pointer) - 2] = '^';
    pointer[sizeof(pointer) - 1] = '\0';

    fprintf(stderr, "%s%s", MESSAGE_STYLE, error->message);

    if (g_filename)
        fprintf(stderr, "%s%s", NORMAL_STYLE " at " FILENAME_STYLE, g_filename);

    fprintf(stderr, NORMAL_STYLE ":\n");

    for (unsigned i = line_from; i <= line_to; ++i)
    {
        fprintf(stderr, "  %*d | %.*s\n", line_width, i + 1, get_line_len(i),
                                          g_lines[i].start);
        if (i == error->line)
            fprintf(stderr, "%s\n", pointer);
    }

    fprintf(stderr, "\n");
}


static int compare_errors(error_t *a, error_t *b)
{
    int res = a->line - b->line;
    return res ? res : a->column - b->column;
}


void print_errors_in_order(int limit)
{
    if (!g_errors || log_mode == LOG_NOTHING)
        return;

    qsort(g_errors, vec_len(g_errors), sizeof(error_t),
        (int (*)(const void *, const void *))compare_errors);

    for (unsigned i = 0; i < vec_len(g_errors) && limit; ++i)
        if (g_errors[i].stylistic || log_mode == LOG_ALL)
        {
            print_error(&g_errors[i]);
            --limit;
        }
}
