#include "clint.h"

enum {NONE, DISALLOWED, REQUIRED};

static int after_keyword = NONE;
static int before_keyword = NONE;
static int after_comma = NONE;
static int after_left_paren = NONE;
static int before_right_paren = NONE;
static int after_left_square = NONE;
static int before_right_square = NONE;
static int before_semicolon = NONE;
static int after_semicolon = NONE;


static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "after-keyword", json_boolean)))
        after_keyword = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "before-keyword", json_boolean)))
        before_keyword = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "after-comma", json_boolean)))
        after_comma = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "after-left-paren", json_boolean)))
        after_left_paren = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "before-right-paren", json_boolean)))
        before_right_paren = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "after-left-square", json_boolean)))
        after_left_square = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "before-right-square", json_boolean)))
        before_right_square = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "before-semicolon", json_boolean)))
        before_semicolon = value->u.boolean ? REQUIRED : DISALLOWED;

    if ((value = json_get(config, "after-semicolon", json_boolean)))
        after_semicolon = value->u.boolean ? REQUIRED : DISALLOWED;
}


static void check_space_before(toknum_t i, int mode)
{
    location_t *start, *prev_end, pos;
    int diff;

    if (!mode)
        return;

    start = &g_tokens[i].start;
    prev_end = &g_tokens[i - 1].end;
    diff = start->pos - prev_end->pos;

    pos = (location_t){prev_end->line, prev_end->column + 1};

    if (mode == REQUIRED)
    {
        if (prev_end->line == start->line)
            if (diff > 2)
                warn_at(&pos, "Missing space before \"%s\"",
                        stringify_kind(g_tokens[i].kind));
            else if (diff < 2)
                warn_at(&pos, "Should be one space instead of %d, before \"%s\"",
                        diff, stringify_kind(g_tokens[i].kind));
    }
    else if (mode == DISALLOWED)
        if (diff > 1)
            warn_at(&pos, "Illegal space before \"%s\"",
                    stringify_kind(g_tokens[i].kind));
}


static void check_space_after(toknum_t i, int mode)
{
    location_t *end, *next_start, pos;
    int diff;

    if (!mode)
        return;

    end = &g_tokens[i].end;
    next_start = &g_tokens[i + 1].start;
    diff = next_start->pos - end->pos;

    pos = (location_t){end->line, end->column + 1};

    if (mode == REQUIRED)
    {
        if (end->line == next_start->line)
            if (diff < 2)
                warn_at(&pos, "Missing space after \"%s\"",
                        stringify_kind(g_tokens[i].kind));
            else if (diff > 2)
                warn_at(&pos, "Should be one space instead of %d, after \"%s\"",
                        diff, stringify_kind(g_tokens[i].kind));
    }
    else if (mode == DISALLOWED)
        if (diff > 1)
            warn_at(&pos, "Illegal space after \"%s\"",
                    stringify_kind(g_tokens[i].kind));
}


static void check()
{
    for (toknum_t i = 2; i < vec_len(g_tokens) - 1; ++i)
        switch (g_tokens[i].kind)
        {
            case KW_WHILE:
            case KW_DO:
            case KW_IF:
            case KW_ELSE:
            case KW_FOR:
            case KW_SWITCH:
                check_space_before(i, before_keyword);
                check_space_after(i, after_keyword);
                break;

            case PN_COMMA:
                if (g_tokens[i + 1].kind != PN_RBRACE &&
                    g_tokens[i + 1].kind != PN_RSQUARE)
                    check_space_after(i, after_comma);
                break;

            case PN_LPAREN:
                check_space_after(i, after_left_paren);
                break;

            case PN_RPAREN:
                check_space_before(i, before_right_paren);
                break;

            case PN_LSQUARE:
                check_space_after(i, after_left_square);
                break;

            case PN_RSQUARE:
                check_space_before(i, before_right_square);
                break;

            case PN_SEMI:
                check_space_before(i, before_semicolon);
                check_space_after(i, after_semicolon);

            default:
                break;
        }
}


REGISTER_RULE(whitespace, configure, check);
