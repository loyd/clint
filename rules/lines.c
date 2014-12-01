#include <ctype.h>
#include <string.h>

#include "clint.h"

// Defaults settings.
static unsigned maximum_length = -1;
static bool disallow_trailing_space = false;
static bool require_newline_at_eof = false;


static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "maximum-length", json_integer)))
        if (value->u.integer > 0)
            maximum_length = value->u.integer;

    if ((value = json_get(config, "disallow-trailing-space", json_boolean)))
        disallow_trailing_space = value->u.boolean;

    if ((value = json_get(config, "require-newline-at-eof", json_boolean)))
        require_newline_at_eof = value->u.boolean;
}


static unsigned get_line_size(unsigned line)
{
    if (line + 1 == vec_len(g_lines))
        return strlen(g_lines[line].start);
    else
        return g_lines[line + 1].start - g_lines[line].start - 1;
}


static unsigned get_line_len(unsigned line, unsigned size)
{
    unsigned len = 0;
    char *start = g_lines[line].start;

    for (unsigned i = 0; i < size; ++i)
        if ((start[i] & 0xc0) != 0x80)
            ++len;

    return len;
}


static void check(void)
{
    unsigned line = 0;
    unsigned size;

    for (; line < vec_len(g_lines); ++line)
    {
        if ((size = get_line_size(line)) == 0)
            continue;

        if (disallow_trailing_space && isspace(g_lines[line].start[size-1]))
        {
            unsigned column = size - 1;

            while (column && isspace(g_lines[line].start[column]))
                --column;

            add_warn(line, column + 1, "Trailing witespaces are disallowed");
        }

        if (size > maximum_length && get_line_len(line, size) > maximum_length)
            add_warn(line, maximum_length,
                     "Line must be at most %d characters", maximum_length);
    }

    if (require_newline_at_eof)
        if (size > 0)
            add_warn(line - 1, size, "Required newline at eof.");
}


REGISTER_RULE(lines, configure, check);
