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

enum log_mode_e g_log_mode = LOG_SORTED|LOG_COLOR;
unsigned g_log_limit = -1;


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

    if (g_lines[line].length)
        return g_lines[line].length;

    // The following line isn't parsed.
    return strpbrk(g_lines[line].start, "\n\r\0") - g_lines[line].start;
}


#ifdef _WIN32
#   define MESSAGE_STYLE ""
#   define FILENAME_STYLE ""
#   define NORMAL_STYLE ""
#else
#   define MESSAGE_STYLE   "\x1b[1m"
#   define FILENAME_STYLE  "\x1b[32;1m"
#   define NORMAL_STYLE    "\x1b[0m"
#endif

static void print_error(error_t *error)
{
    if (g_log_mode & LOG_SHORTLY)
    {
        fprintf(stderr, MESSAGE_STYLE "%s" NORMAL_STYLE " at "
                        FILENAME_STYLE "%s" NORMAL_STYLE " (%u:%u)\n",
            error->message, g_filename, error->line, error->column);

        return;
    }

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


void add_log(bool style, unsigned line, unsigned column, const char *fmt, ...)
{
    va_list arg;
    int len;
    char *msg;

    if (!(style || g_log_mode & LOG_VERBOSE))
        return;

    if (!g_errors)
        g_errors = new_vec(error_t, 24);

    if (vec_len(g_errors) >= g_log_limit)
        return;

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

    if (!(g_log_mode & (LOG_SORTED|LOG_SILENCE)))
        print_error(&g_errors[vec_len(g_errors) - 1]);
}


static int compare_errors(error_t *a, error_t *b)
{
    int res = a->line - b->line;
    return res ? res : a->column - b->column;
}


void print_errors_in_order(void)
{
    if (!g_errors || g_log_mode & LOG_SILENCE)
        return;

    qsort(g_errors, vec_len(g_errors), sizeof(error_t),
        (int (*)(const void *, const void *))compare_errors);

    for (unsigned i = 0; i < vec_len(g_errors); ++i)
        print_error(&g_errors[i]);
}
