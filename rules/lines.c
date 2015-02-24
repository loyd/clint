#include <ctype.h>
#include <string.h>

#include "clint.h"

// Defaults settings.
static unsigned maximum_length;
static bool disallow_trailing_space;
static bool require_newline_at_eof;
static const char *line_break;


static void configure(void)
{
    if ((maximum_length = cfg_natural("maximum-length")) == 0)
        maximum_length = -1;

    disallow_trailing_space = cfg_boolean("disallow-trailing-space");
    line_break = cfg_string("require-line-break");
    require_newline_at_eof = cfg_boolean("require-newline-at-eof");
}


static unsigned utf8_len(const char *line, unsigned size)
{
    unsigned length = 0;

    for (unsigned i = 0; i < size; ++i)
        if ((line[i] & 0xc0) != 0x80)
            ++length;

    return length;
}


static void check(void)
{
    bool check_lb = !!line_break;
    unsigned lb_length = check_lb ? strlen(line_break) : 0;
    unsigned i = 0;
    const char *line;
    unsigned length;

    for (; i < vec_len(g_lines); ++i)
    {
        line = g_lines[i].start;
        length = g_lines[i].length;

        if (length == 0)
            continue;

        if (disallow_trailing_space && isspace(line[length - 1]))
        {
            unsigned column = length - 1;

            while (column && isspace(line[column]))
                --column;

            add_warn(i, column + 1, "Trailing witespaces are disallowed");
        }

        if (length > maximum_length && utf8_len(line, length) > maximum_length)
            add_warn(i, maximum_length,
                     "Line must be at most %d characters", maximum_length);

        if (check_lb && line[length])
            if (strncmp(line + length, line_break, lb_length))
            {
                add_warn(i, length, "Invalid line break");
                check_lb = false;
            }
    }

    if (require_newline_at_eof && g_lines[i - 1].length > 0)
        add_warn(i - 1, length, "Required newline at eof");
}


REGISTER_RULE(lines, configure, check);
