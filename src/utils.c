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


void *xrealloc(void *ptr, size_t size)
{
    assert(size > 0);

    ptr = realloc(ptr, size);
    if (!ptr)
        abort();

    return ptr;
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
    assert(num >= 0);
    unsigned res = 0;

    while (num)
    {
        ++res;
        num /= 10;
    }

    return res;
}


static inline unsigned get_line_len(unsigned line)
{
    assert(line < vec_len(g_lines));

    if (line+1 == vec_len(g_lines))
    {
        char *nl = strchr(g_lines[line], '\n');
        return nl ? nl - g_lines[line] : strlen(g_lines[line]);
    }

    return g_lines[line+1] - g_lines[line] - 1;
}


#define MESSAGE_STYLE   "\x1b[1m"
#define FILENAME_STYLE  "\x1b[32;1m"
#define NORMAL_STYLE    "\x1b[0m"

static bool flowing = true;
void resume_warnings(void) { flowing = true; }
void pause_warnings(void) { flowing = false; }


void *warn_at(unsigned line, unsigned column, const char *format, ...)
{
    assert(format);
    assert(line < vec_len(g_lines));

    unsigned line_len = get_line_len(line);
    assert(column <= line_len);

    if (!flowing)
        return NULL;

    va_list arg;
    unsigned line_from = line-2;
    unsigned line_to = line+2;

    if (line_to >= vec_len(g_lines))
        line_to = vec_len(g_lines) - 1;

    unsigned line_width = count_signs(line_to);

    char pointer[column + line_width + 7];
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
                                          g_lines[i]);
        if (i == line)
            fprintf(stderr, "%s\n", pointer);
    }

    fprintf(stderr, "\n");

    return NULL;
}
