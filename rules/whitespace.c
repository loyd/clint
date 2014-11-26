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
static int require_block_on_newline = NONE;
static int newline_before_block = NONE;
static int newline_before_else = NONE;
static int newline_before_fn_body = NONE;
static int empty_around_fn_def = NONE;


static void configure(json_value *config)
{
    json_value *value;

#define bind_option(name, string)                                             \
    if ((value = json_get(config, string, json_boolean)))                     \
        name = value->u.boolean + 1

    bind_option(after_keyword,            "after-keyword");
    bind_option(before_keyword,           "before-keyword");
    bind_option(after_comma,              "after-comma");
    bind_option(after_left_paren,         "after-left-paren");
    bind_option(before_right_paren,       "before-right-paren");
    bind_option(after_left_square,        "after-left-square");
    bind_option(before_right_square,      "before-right-square");
    bind_option(before_semicolon,         "before-semicolon");
    bind_option(after_semicolon,          "after-semicolon");
    bind_option(require_block_on_newline, "require-block-on-newline");
    bind_option(newline_before_block,     "newline-before-block");
    bind_option(newline_before_else,      "newline-before-else");
    bind_option(newline_before_fn_body,   "newline-before-fn-body");
    bind_option(empty_around_fn_def,      "empty-around-fn-def");
}


static void check_space_before(toknum_t i, int mode)
{
    location_t *start, *prev_end;
    const char *msg = NULL;
    int diff;

    if (!mode)
        return;

    start = &g_tokens[i].start;
    prev_end = &g_tokens[i - 1].end;
    diff = start->pos - prev_end->pos;

    if (mode == REQUIRED)
    {
        if (prev_end->line == start->line)
            if (diff > 2)
                msg = "Missing space before \"%s\"";
            else if (diff < 2)
                msg = "Should be only one space before \"%s\"";
    }
    else if (mode == DISALLOWED)
        if (diff > 1)
            msg = "Illegal space before \"%s\"";

    if (msg)
        warn_at(&(location_t){prev_end->line, prev_end->column + 1},
                msg, stringify_kind(g_tokens[i].kind));
}


static void check_space_after(toknum_t i, int mode)
{
    location_t *end, *next_start;
    const char *msg = NULL;
    int diff;

    if (!mode)
        return;

    end = &g_tokens[i].end;
    next_start = &g_tokens[i + 1].start;
    diff = next_start->pos - end->pos;

    if (mode == REQUIRED)
    {
        if (end->line == next_start->line)
            if (diff < 2)
                msg = "Missing space after \"%s\"";
            else if (diff > 2)
                msg = "Should be only one space after \"%s\"";
    }
    else if (mode == DISALLOWED)
        if (diff > 1)
            msg = "Illegal space after \"%s\"";

    if (msg)
        warn_at(&(location_t){end->line, end->column + 1},
                msg, stringify_kind(g_tokens[i].kind));
}


static void check_newline_before(toknum_t i, int mode)
{
    const char *msg = NULL;

    if (!mode)
        return;

    if (g_tokens[i].start.line == g_tokens[i - 1].end.line)
    {
        if (mode == REQUIRED)
            msg = "Missing newline before \"%s\"";
    }
    else
        if (mode == DISALLOWED)
            msg = "Newline before \"%s\" is disallowed";

    if (msg)
        warn_at(&g_tokens[i].start, msg, stringify_kind(g_tokens[i].kind));
}


static void check_newline_after(toknum_t i, int mode)
{
    const char *msg = NULL;

    if (!mode)
        return;

    if (g_tokens[i].end.line == g_tokens[i + 1].start.line)
    {
        if (mode == REQUIRED)
            msg = "Missing newline after \"%s\"";
    }
    else
        if (mode == DISALLOWED)
            msg = "Newline after \"%s\" is disallowed";

    if (msg)
        warn_at(&g_tokens[i].end, msg, stringify_kind(g_tokens[i].kind));
}


static void check_tokens(void)
{
    for (toknum_t i = 2; i < vec_len(g_tokens) - 1; ++i)
        switch (g_tokens[i].kind)
        {
            case KW_ELSE:
                check_newline_before(i, newline_before_else);
            case KW_WHILE:
            case KW_DO:
            case KW_IF:
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


static bool process_block(struct block_s *tree)
{
    check_newline_after(tree->start, require_block_on_newline);
    check_newline_before(tree->end, require_block_on_newline);

    check_newline_before(tree->start,
        tree->parent->type == FUNCTION_DEF ? newline_before_fn_body
                                           : newline_before_block);

    return true;
}


static void check(void)
{
    check_tokens();
    iterate_by_type(BLOCK, process_block);
}



REGISTER_RULE(whitespace, configure, check);
