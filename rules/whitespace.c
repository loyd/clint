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
static int newline_before_members = NONE;
static int newline_before_block = NONE;
static int newline_before_else = NONE;
static int newline_before_fn_body = NONE;
static int between_unary_and_operand = NONE;
static int around_binary = NONE;
static int around_assignment = NONE;
static int around_accessor = NONE;
static int in_conditional = NONE;
static int after_cast = NONE;
static int in_call = NONE;
static int after_name_in_fn_def = NONE;
static int before_declarator_name = NONE;
static int before_members = NONE;


static void configure(json_value *config)
{
    json_value *value;

#define bind_option(name, string)                                             \
    if ((value = json_get(config, string, json_boolean)))                     \
        name = value->u.boolean + 1

    bind_option(after_keyword,             "after-keyword");
    bind_option(before_keyword,            "before-keyword");
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
    bind_option(newline_before_else,       "newline-before-else");
    bind_option(newline_before_fn_body,    "newline-before-fn-body");
    bind_option(between_unary_and_operand, "between-unary-and-operand");
    bind_option(around_binary,             "around-binary");
    bind_option(around_assignment,         "around-assignment");
    bind_option(around_accessor,           "around-accessor");
    bind_option(in_conditional,            "in-conditional");
    bind_option(after_cast,                "after-cast");
    bind_option(in_call,                   "in-call");
    bind_option(after_name_in_fn_def,      "after-name-in-fn-def");
    bind_option(before_declarator_name,    "before-declarator-name");
    bind_option(before_members,            "before-members");
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
        warn_at(&(location_t){prev_end->line, prev_end->column + 1},
                msg, where);
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
        warn_at(&(location_t){end->line, end->column + 1}, msg, where);
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
        warn_at(&g_tokens[i].start, msg, where);
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
        warn_at(&g_tokens[i].end, msg, where);
}


static void check_tokens(void)
{
    for (toknum_t i = 2; i < vec_len(g_tokens) - 1; ++i)
        switch (g_tokens[i].kind)
        {
            case KW_ELSE:
                check_newline_before(i, newline_before_else, "keyword");
            case KW_WHILE:
            case KW_DO:
            case KW_IF:
            case KW_FOR:
            case KW_SWITCH:
                check_space_before(i, before_keyword, "keyword");
                check_space_after(i, after_keyword, "keyword");
                break;

            case KW_STRUCT:
            case KW_UNION:
            case KW_ENUM:
                check_space_after(i, after_keyword, "keyword");
                break;

            case PN_COMMA:
                if (g_tokens[i + 1].kind != PN_RBRACE &&
                    g_tokens[i + 1].kind != PN_RSQUARE)
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


static bool process_block(struct block_s *tree)
{
    check_newline_after(tree->start, require_block_on_newline, "block");
    check_newline_before(tree->end, require_block_on_newline, "block");

    if (tree->parent->type == FUNCTION_DEF)
    {
        struct function_def_s *fn_def = (void *)tree->parent;
        toknum_t name = ((struct declarator_s *)fn_def->decl)->name;

        // Case 'int (name)(...) {...}'.
        if (g_tokens[name + 1].kind == PN_RPAREN)
            ++name;

        check_space_after(name, after_name_in_fn_def, "function name");
        check_newline_before(tree->start, newline_before_fn_body, "body");
    }
    else
        check_newline_before(tree->start, newline_before_block, "block");

    return true;
}


static bool process_unary(struct unary_s *tree)
{
    int mode = between_unary_and_operand;

    if (tree->op < tree->expr->start)
        check_space_after(tree->op, mode, "unary operator");
    else
        check_space_before(tree->op, mode, "unary operator");

    return true;
}


static bool process_binary(struct binary_s *tree)
{
    check_space_before(tree->op, around_binary, "binary operator");
    check_space_after(tree->op, around_binary, "binary operator");

    return true;
}


static bool process_assignment(struct assignment_s *tree)
{
    check_space_before(tree->op, around_assignment, "assignment");
    check_space_after(tree->op, around_assignment, "assignment");

    return true;
}


static bool process_accessor(struct accessor_s *tree)
{
    check_space_before(tree->op, around_accessor, "field accessor");
    check_space_after(tree->op, around_accessor, "field accessor");

    return true;
}


static bool process_conditional(struct conditional_s *tree)
{
    check_space_after(tree->cond->end, in_conditional, "test");
    check_space_before(tree->then_br->start, in_conditional, "consequent");
    check_space_after(tree->then_br->end, in_conditional, "consequent");
    check_space_before(tree->else_br->start, in_conditional, "alternate");

    return true;
}


static bool process_cast(struct cast_s *tree)
{
    check_space_after(tree->type_name->end + 1, after_cast, "cast");
    return true;
}


static bool process_call(struct call_s *tree)
{
    check_space_before(tree->left->end + 1, in_call, "call");
    return true;
}


static bool process_declarator(struct declarator_s *tree)
{
    if (!tree->name)
        return false;

    check_space_before(tree->name, before_declarator_name, "declarator name");
    return false;
}


static bool process_specifiers(struct specifiers_s *tree)
{
    tree_t dirtype = tree->dirtype;

    if (!dirtype)
        return false;

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
    else
        return false;

    return true;
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
}


REGISTER_RULE(whitespace, configure, check);
