#include <ctype.h>
#include <string.h>

#include "clint.h"

// Defaults settings.
static unsigned maximum_length = -1;
static bool disallow_trailing_space = false;
static bool require_newline_at_eof = false;
static const char *line_break = NULL;
static unsigned line_break_length;


static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "maximum-length", json_integer)))
        if (value->u.integer > 0)
            maximum_length = value->u.integer;

    if ((value = json_get(config, "disallow-trailing-space", json_boolean)))
        disallow_trailing_space = value->u.boolean;

    if ((value = json_get(config, "require-line-break", json_string)))
    {
        line_break = value->u.string.ptr;
        line_break_length = value->u.string.length;
    }

    if ((value = json_get(config, "require-newline-at-eof", json_boolean)))
        require_newline_at_eof = value->u.boolean;
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
            if (strncmp(line + length, line_break, line_break_length))
            {
                add_warn(i, length, "Invalid line break");
                check_lb = false;
            }
    }

    if (require_newline_at_eof && g_lines[i - 1].length > 0)
        add_warn(i - 1, length, "Required newline at eof");
}


REGISTER_RULE(lines, configure, check);
