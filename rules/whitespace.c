#include <string.h>

#include "clint.h"

enum {NONE, DISALLOWED, REQUIRED};

static int after_control = NONE;
static int before_control = NONE;
static int before_comma = NONE;
static int after_comma = NONE;
static int after_left_paren = NONE;
static int before_right_paren = NONE;
static int after_left_square = NONE;
static int before_right_square = NONE;
static int before_semicolon = NONE;
static int after_semicolon = NONE;
static int require_block_on_newline = NONE;
static int newline_before_members = NONE;
static int newline_before_block = NONE;
static int newline_before_control = NONE;
static int newline_before_fn_body = NONE;
static int between_unary_and_operand = NONE;
static int around_binary = NONE;
static int around_bitwise = NONE;
static int around_assignment = NONE;
static int around_accessor = NONE;
static int in_conditional = NONE;
static int after_cast = NONE;
static int in_call = NONE;
static int after_name_in_fn_def = NONE;
static int before_declarator_name = NONE;
static int before_members = NONE;

static bool allow_alignment = false;

static enum {FREE, MIDDLE, TYPE, DECL} pointer_place = FREE;


static void configure(json_value *config)
{
    json_value *value;

#define bind_option(name, string)                                             \
    if ((value = json_get(config, string, json_boolean)))                     \
        name = value->u.boolean + 1

    bind_option(after_control,             "after-control");
    bind_option(before_control,            "before-control");
    bind_option(before_comma,              "before-comma");
    bind_option(after_comma,               "after-comma");
    bind_option(after_left_paren,          "after-left-paren");
    bind_option(before_right_paren,        "before-right-paren");
    bind_option(after_left_square,         "after-left-square");
    bind_option(before_right_square,       "before-right-square");
    bind_option(before_semicolon,          "before-semicolon");
    bind_option(after_semicolon,           "after-semicolon");
    bind_option(require_block_on_newline,  "require-block-on-newline");
    bind_option(newline_before_members,    "newline-before-members");
    bind_option(newline_before_block,      "newline-before-block");
    bind_option(newline_before_control,    "newline-before-control");
    bind_option(newline_before_fn_body,    "newline-before-fn-body");
    bind_option(between_unary_and_operand, "between-unary-and-operand");
    bind_option(around_binary,             "around-binary");
    bind_option(around_bitwise,            "around-bitwise");
    bind_option(around_assignment,         "around-assignment");
    bind_option(around_accessor,           "around-accessor");
    bind_option(in_conditional,            "in-conditional");
    bind_option(after_cast,                "after-cast");
    bind_option(in_call,                   "in-call");
    bind_option(after_name_in_fn_def,      "after-name-in-fn-def");
    bind_option(before_declarator_name,    "before-declarator-name");
    bind_option(before_members,            "before-members");

    if ((value = json_get(config, "allow-alignment", json_boolean)))
        allow_alignment = value->u.boolean;

    if ((value = json_get(config, "pointer-place", json_string)))
        if (!strcmp("declarator", value->u.string.ptr))
            pointer_place = DECL;
        else if (!strcmp("type", value->u.string.ptr))
            pointer_place = TYPE;
        else if (!strcmp("middle", value->u.string.ptr))
            pointer_place = MIDDLE;
}


static void check_space_before(toknum_t i, int mode, const char *where)
{
    location_t *start, *prev_end;
    const char *msg = NULL;
    int diff;

    if (!mode)
        return;

    start = &g_tokens[i].start;
    prev_end = &g_tokens[i - 1].end;
    diff = start->pos - prev_end->pos;

    if (prev_end->line != start->line)
        return;

    if (mode == REQUIRED)
    {
        if (diff < 2)
            msg = "Missing space before %s";
        else if (diff > 2)
            msg = "Should be only one space before %s";
    }
    else if (mode == DISALLOWED)
        if (diff > 1)
            msg = "Illegal space before %s";

    if (msg)
        add_warn(prev_end->line, prev_end->column + 1, msg, where);
}


static void check_space_after(toknum_t i, int mode, const char *where)
{
    location_t *end, *next_start;
    const char *msg = NULL;
    int diff;

    if (!mode)
        return;

    end = &g_tokens[i].end;
    next_start = &g_tokens[i + 1].start;
    diff = next_start->pos - end->pos;

    if (end->line != next_start->line)
        return;

    if (mode == REQUIRED)
    {
        if (diff < 2)
            msg = "Missing space after %s";
        else if (diff > 2)
            msg = "Should be only one space after %s";
    }
    else if (mode == DISALLOWED)
        if (diff > 1)
            msg = "Illegal space after %s";

    if (msg)
        add_warn(end->line, end->column + 1, msg, where);
}


static void check_newline_before(toknum_t i, int mode, const char *where)
{
    const char *msg = NULL;

    if (!mode)
        return;

    if (g_tokens[i].start.line == g_tokens[i - 1].end.line)
    {
        if (mode == REQUIRED)
            msg = "Missing newline before %s";
    }
    else
        if (mode == DISALLOWED)
            msg = "Newline before %s is disallowed";

    if (msg)
        add_warn_at(g_tokens[i].start, msg, where);
}


static void check_newline_after(toknum_t i, int mode, const char *where)
{
    const char *msg = NULL;

    if (!mode)
        return;

    if (g_tokens[i].end.line == g_tokens[i + 1].start.line)
    {
        if (mode == REQUIRED)
            msg = "Missing newline after %s";
    }
    else
        if (mode == DISALLOWED)
            msg = "Newline after %s is disallowed";

    if (msg)
        add_warn_at(g_tokens[i].end, msg, where);
}


static char ch_from(unsigned line, unsigned column)
{
    return column < g_lines[line].length ? g_lines[line].start[column] : '\0';
}


static bool same_top_or_bottom(unsigned line, unsigned column)
{
    char ch = ch_from(line, column);
    return ch == ch_from(line - 1, column) ||
           ch == ch_from(line + 1, column);
}


static bool is_aligned(toknum_t i)
{
    token_t *tok, *prev, *next;
    unsigned line, column;

    if (!allow_alignment)
        return false;

    tok = &g_tokens[i];
    prev = &g_tokens[i - 1];
    next = &g_tokens[i + 1];
    line = tok->start.line;
    column = tok->start.column;

    if (tok->start.line != prev->end.line ||
        tok->start.column - prev->end.column < 2)
        return false;

    switch (tok->kind)
    {
        // a,  "a"
        // ab, "ab"
        case TOK_CHAR_CONST:
        case TOK_STRING:
        {
            char quote;

            if (ch_from(line, column) == 'L')
                ++column;

            quote = ch_from(line, column);

            if (quote == ch_from(line - 1, column) ||
                quote == ch_from(line + 1, column))
                return true;

            break;
        }

        // ab < a
        // a  < c
        case PN_EQ:
        case PN_CARET:
        case PN_AMP: case PN_PIPE:
        case PN_GT: case PN_LE:
        case PN_QUESTION: case PN_COLON:
        case PN_PLUS: case PN_MINUS:
        case PN_STAR: case PN_SLASH: case PN_PERCENT:
            if (same_top_or_bottom(line, column))
                return true;
            break;

        // ab  = c
        // ab += d
        case PN_PLUSEQ: case PN_MINUSEQ:
        case PN_STAREQ: case PN_SLASHEQ: case PN_PERCENTEQ:
        case PN_CARETEQ: case PN_LELEEQ: case PN_GTGTEQ:
        case PN_AMPEQ: case PN_PIPEEQ:
            if (same_top_or_bottom(line, tok->end.column))
                return true;

            // Fallthrough.

        // ab << a
        // a  << c
        case PN_LEEQ: case PN_GTEQ: case PN_EXCLAIMEQ:
        case PN_AMPAMP: case PN_PIPEPIPE:
        case PN_LELE: case PN_GTGT:
            if (same_top_or_bottom(line, column) &&
                same_top_or_bottom(line, column + 1))
                return true;
            break;

        default:
            break;
    }

    // a,  NULL }
    // ab, "bc" }
    //    and
    //  a,
    // ab,
    if ((next->kind == PN_COMMA || next->kind == PN_RBRACE) &&
        next->start.line == line)
    {
        if (same_top_or_bottom(next->start.line, next->start.column))
            return true;
    }

    return false;
}


static void check_tokens(void)
{
    for (toknum_t i = 2; i < vec_len(g_tokens) - 1; ++i)
        switch (g_tokens[i].kind)
        {
            case KW_IF:
            case KW_ELSE:
            case KW_WHILE:
            case KW_DO:
            case KW_FOR:
            case KW_SWITCH:
                // Case "else if".
                if (g_tokens[i].kind == KW_IF && g_tokens[i-1].kind != KW_ELSE)
                    check_newline_before(i, newline_before_control, "control");

                check_space_before(i, before_control, "control");
                check_space_after(i, after_control, "control");
                break;

            case KW_STRUCT:
            case KW_UNION:
            case KW_ENUM:
                check_space_after(i, after_control, "keyword");
                break;

            case PN_COMMA:
                check_space_before(i, before_comma, "comma");

                if (g_tokens[i + 1].kind != PN_RBRACE &&
                    g_tokens[i + 1].kind != PN_RSQUARE &&
                    !is_aligned(i + 1))
                    check_space_after(i, after_comma, "comma");
                break;

            case PN_LPAREN:
                check_space_after(i, after_left_paren, "parenthesis");
                break;

            case PN_RPAREN:
                check_space_before(i, before_right_paren, "parenthesis");
                break;

            case PN_LSQUARE:
                check_space_after(i, after_left_square, "parenthesis");
                break;

            case PN_RSQUARE:
                check_space_before(i, before_right_square, "parenthesis");
                break;

            case PN_SEMI:
                if (g_tokens[i + 1].kind != PN_LPAREN &&
                    g_tokens[i + 1].kind != PN_SEMI)
                    check_space_before(i, before_semicolon, "semicolon");

                if (g_tokens[i + 1].kind != PN_RPAREN &&
                    g_tokens[i + 1].kind != PN_SEMI)
                    check_space_after(i, after_semicolon, "semicolon");

            default:
                break;
        }
}


static void process_block(struct block_s *tree)
{
    check_newline_after(tree->start, require_block_on_newline, "block");
    check_newline_before(tree->end, require_block_on_newline, "block");

    if (tree->parent->type == FUNCTION_DEF)
    {
        struct function_def_s *fn_def = (void *)tree->parent;
        toknum_t name = ((struct declarator_s *)fn_def->decl)->name;

        // Case "int (name)(...) {...}".
        if (g_tokens[name + 1].kind == PN_RPAREN)
            ++name;

        check_space_after(name, after_name_in_fn_def, "function name");
        check_newline_before(tree->start, newline_before_fn_body, "body");
    }
    else
        check_newline_before(tree->start, newline_before_block, "block");
}


static void process_unary(struct unary_s *tree)
{
    int mode = between_unary_and_operand;

    if (g_tokens[tree->op].kind == KW_SIZEOF)
    {
        if (g_tokens[tree->op + 1].kind == PN_LPAREN)
            check_space_before(tree->op + 1, in_call, "call");

        return;
    }

    if (tree->op < tree->expr->start)
        check_space_after(tree->op, mode, "unary operator");
    else
        check_space_before(tree->op, mode, "unary operator");
}


static void process_binary(struct binary_s *tree)
{
    enum token_e kind = g_tokens[tree->op].kind;

    if (kind == PN_PIPE || kind == PN_AMP)
    {
        if (!is_aligned(tree->op))
            check_space_before(tree->op, around_bitwise, "bitwise operator");
        check_space_after(tree->op, around_bitwise, "bitwise operator");
    }
    else
    {
        if (!is_aligned(tree->op))
            check_space_before(tree->op, around_binary, "binary operator");
        check_space_after(tree->op, around_binary, "binary operator");
    }
}


static void process_assignment(struct assignment_s *tree)
{
    if (!is_aligned(tree->op))
        check_space_before(tree->op, around_assignment, "assignment");
    check_space_after(tree->op, around_assignment, "assignment");
}


static void process_accessor(struct accessor_s *tree)
{
    check_space_before(tree->op, around_accessor, "field accessor");
    check_space_after(tree->op, around_accessor, "field accessor");
}


static toknum_t find_tok(enum token_e kind, toknum_t from, toknum_t to)
{
    for (toknum_t i = from; i < to; ++i)
        if (g_tokens[i].kind == kind)
            return i;

    return 0;
}


static void process_conditional(struct conditional_s *tree)
{
    toknum_t quest, colon;

    if (!in_conditional)
        return;

    quest = find_tok(PN_QUESTION, tree->cond->end, tree->then_br->start);
    colon = find_tok(PN_COLON, tree->then_br->end, tree->else_br->start);

    if (!is_aligned(quest))
        check_space_after(quest - 1, in_conditional, "test");

    check_space_before(quest + 1, in_conditional, "consequent");

    if (!is_aligned(colon))
        check_space_after(colon - 1, in_conditional, "consequent");

    check_space_before(colon + 1, in_conditional, "alternate");
}


static void process_cast(struct cast_s *tree)
{
    check_space_after(tree->type_name->end + 1, after_cast, "cast");
}


static void process_call(struct call_s *tree)
{
    check_space_before(tree->left->end + 1, in_call, "call");
}


static void process_declarator(struct declarator_s *tree)
{
    if (!tree->name ||
        g_tokens[tree->name - 1].kind == PN_LPAREN ||
        g_tokens[tree->name - 1].kind == PN_STAR)
        return;

    check_space_before(tree->name, before_declarator_name, "declarator name");
}


static void process_specifiers(struct specifiers_s *tree)
{
    tree_t dirtype = tree->dirtype;

    if (!dirtype)
        return;

    if (dirtype->type == ENUM)
    {
        struct enum_s *cmplx = (void *)dirtype;
        if (cmplx->values)
        {
            toknum_t st = (cmplx->name ? cmplx->name : cmplx->start) + 1;
            check_newline_before(st, newline_before_members, "values");
            check_space_before(st, before_members, "values");
        }
    }
    else if (dirtype->type == STRUCT || dirtype->type == UNION)
    {
        struct struct_s *cmplx = (void *)dirtype;
        if (cmplx->members)
        {
            toknum_t st = (cmplx->name ? cmplx->name : cmplx->start) + 1;
            check_newline_before(st, newline_before_members, "members");
            check_space_before(st, before_members, "members");
        }
    }
}


static void process_pointer(struct pointer_s *tree)
{
    int before = (pointer_place != TYPE) + 1;
    int after = (pointer_place != DECL) + 1;
    toknum_t place = tree->specs ? tree->specs->end : tree->start;
    enum token_e next = g_tokens[place + 1].kind;
    enum token_e prev = g_tokens[tree->start - 1].kind;

    if (tree->specs)
        check_space_before(tree->specs->start, DISALLOWED, "qualifier");

    if (next == PN_STAR)
        check_space_after(place, tree->specs ? REQUIRED : DISALLOWED,
                          "pointer");

    if (!(tree->parent->type == POINTER || prev == PN_LPAREN))
        check_space_before(tree->start, before, "pointer");

    if (!(next == PN_STAR || next == PN_RPAREN || next == PN_COMMA))
        check_space_after(place, after, "pointer");
}


static void check(void)
{
    check_tokens();
    iterate_by_type(BLOCK, process_block);
    iterate_by_type(UNARY, process_unary);
    iterate_by_type(BINARY, process_binary);
    iterate_by_type(ASSIGNMENT, process_assignment);
    iterate_by_type(ACCESSOR, process_accessor);
    iterate_by_type(CONDITIONAL, process_conditional);
    iterate_by_type(CAST, process_cast);
    iterate_by_type(CALL, process_call);
    iterate_by_type(DECLARATOR, process_declarator);
    iterate_by_type(SPECIFIERS, process_specifiers);

    if (pointer_place != FREE)
        iterate_by_type(POINTER, process_pointer);
}


REGISTER_RULE(whitespace, configure, check);
