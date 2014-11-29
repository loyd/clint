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


#define MESSAGE_STYLE   "\x1b[1m"
#define FILENAME_STYLE  "\x1b[32;1m"
#define NORMAL_STYLE    "\x1b[0m"

static enum log_level_e log_level = LOG_WARNING;

void set_log_level(enum log_level_e level)
{
    log_level = level;
}


void *log_at(enum log_level_e level, location_t *loc, const char *format, ...)
{
    assert(level > LOG_SILENCE);
    assert(format);
    assert(loc->line < vec_len(g_lines));

    if (log_level < level)
        return NULL;

    unsigned line_len = get_line_len(loc->line);
    assert(loc->column <= line_len);

    va_list arg;
    unsigned line_from = loc->line > 2 ? loc->line - 2 : 0;
    unsigned line_to = loc->line+2;

    if (line_to >= vec_len(g_lines))
        line_to = vec_len(g_lines) - 1;

    unsigned line_width = count_signs(line_to + 1);

    char pointer[2 + line_width + 3 + loc->column + 2];
    memset(pointer, '-', sizeof(pointer) - 2);
    pointer[sizeof(pointer) - 2] = '^';
    pointer[sizeof(pointer) - 1] = '\0';

    fprintf(stderr, MESSAGE_STYLE);
    va_start(arg, format);
    vfprintf(stderr, format, arg);
    va_end(arg);

    if (g_filename)
        fprintf(stderr, "%s%s", NORMAL_STYLE " at " FILENAME_STYLE, g_filename);

    fprintf(stderr, NORMAL_STYLE ":\n");

    for (unsigned i = line_from; i <= line_to; ++i)
    {
        fprintf(stderr, "  %*d | %.*s\n", line_width, i+1, get_line_len(i),
                                          g_lines[i].start);
        if (i == loc->line)
            fprintf(stderr, "%s\n", pointer);
    }

    fprintf(stderr, "\n");

    return NULL;
}
