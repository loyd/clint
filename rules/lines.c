#include <ctype.h>
#include <string.h>

#include "clint.h"

// Defaults settings.
static int maximum_length = 0;
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


static int get_line_len(unsigned line)
{
    if (line + 1 == vec_len(g_lines))
        return strlen(g_lines[line].start);
    else
        return g_lines[line + 1].start - g_lines[line].start - 1;
}


static void check(void)
{
    unsigned line = 0;
    int len;

    for (; line < vec_len(g_lines); ++line)
    {
        if ((len = get_line_len(line)) == 0)
            continue;

        if (disallow_trailing_space && isspace(g_lines[line].start[len-1]))
        {
            int column = len - 1;

            while (column && isspace(g_lines[line].start[column]))
                --column;

            warn_at(&(location_t){line, column + 1},
                    "Trailing witespaces are disallowed");
        }

        if (maximum_length && len > maximum_length)
            warn_at(&(location_t){line, maximum_length},
                    "Line must be at most %d characters", maximum_length);
    }

    if (require_newline_at_eof)
        if (len > 0)
            warn_at(&(location_t){line - 1, len}, "Required newline at eof.");
}


REGISTER_RULE(lines, configure, check);
