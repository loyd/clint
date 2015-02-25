#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "clint.h"

static char *global_var_prefix;
static char *global_fn_prefix;
static char *typedef_suffix;
static char *struct_suffix;
static char *union_suffix;
static char *enum_suffix;
static enum {NONE, UNDER_SCORE} style;
static int minimum_length;
static bool allow_short_on_top;
static bool allow_short_in_loop;
static bool allow_short_in_block;
static bool disallow_leading_underscore;


static void configure(void)
{
    char *require_style = cfg_string("require-style");
    if (!require_style || !strcmp("none", require_style))
        style = NONE;
    else if (!strcmp("under_score", require_style))
        style = UNDER_SCORE;
    else
        cfg_fatal("require-style", "contains incorrect style");

    global_var_prefix = cfg_string("global-var-prefix");
    global_fn_prefix = cfg_string("global-fn-prefix");
    typedef_suffix = cfg_string("typedef-suffix");
    struct_suffix = cfg_string("struct-suffix");
    union_suffix = cfg_string("union-suffix");
    enum_suffix = cfg_string("enum-suffix");
    minimum_length = cfg_natural("minimum-length");
    allow_short_on_top = cfg_boolean("allow-short-on-top");
    allow_short_in_loop = cfg_boolean("allow-short-in-loop");
    allow_short_in_block = cfg_boolean("allow-short-in-block");
    disallow_leading_underscore = cfg_boolean("disallow-leading-underscore");
}


static void check_name(toknum_t toknum, bool strict, char *prefix, char *suffix)
{
    token_t *token;
    int plen = 0, slen = 0;

    if (!toknum)
        return;

    token = &g_tokens[toknum];

    if (prefix)
    {
        plen = strlen(prefix);
        if (memcmp(token->start.pos, prefix, plen))
            add_warn_at(token->start, "Required \"%s\" prefix", prefix);
    }

    if (suffix)
    {
        slen = strlen(suffix);
        if (memcmp(token->end.pos - slen + 1, suffix, slen))
            add_warn_at(token->end, "Required \"%s\" suffix", suffix);
    }

    if (disallow_leading_underscore)
        if (token->start.pos[0] == '_')
            add_warn_at(token->start, "Leading underscore is disallowed");

    if (style == UNDER_SCORE)
        for (char *pos = token->start.pos + plen;
             pos <= token->end.pos - slen; ++pos)
            if (!(islower(*pos) || isdigit(*pos) || *pos == '_'))
            {
                add_warn_at(token->start, "Required under_score style");
                break;
            }

    if (strict && minimum_length)
        if (token->end.pos - token->start.pos + 1 < minimum_length)
            add_warn_at(token->start, "Identifier should be at least %d",
                        minimum_length);
}


static void check_dirtype(tree_t dirtype, bool strict)
{
    if (!dirtype)
        return;

    if (dirtype->type == STRUCT)
    {
        struct struct_s *sbj = (void *)dirtype;

        if (sbj->members)
            check_name(sbj->name, strict, NULL, struct_suffix);
    }
    else if (dirtype->type == UNION)
    {
        struct union_s *sbj = (void *)dirtype;

        if (sbj->members)
            check_name(sbj->name, strict, NULL, union_suffix);
    }
    else if (dirtype->type == ENUM)
    {
        struct enum_s *sbj = (void *)dirtype;

        if (sbj->values)
            check_name(sbj->name, strict, NULL, enum_suffix);
    }
}


static void process_decl(struct declaration_s *tree)
{
    struct specifiers_s *specs = (void *)tree->specs;
    bool is_global, is_typedef, strict;

    if (!specs || g_tokens[specs->storage].kind == KW_EXTERN)
        return;

    is_global = tree->parent->type == TRANSL_UNIT && !specs->storage;
    is_typedef = g_tokens[specs->storage].kind == KW_TYPEDEF;

    strict = !(allow_short_on_top && is_global ||
               allow_short_in_loop && tree->parent->type == FOR ||
               allow_short_in_block && !is_global);

    check_dirtype(specs->dirtype, strict);

    if (!tree->decls)
        return;

    for (unsigned i = 0; i < vec_len(tree->decls); ++i)
    {
        struct declarator_s *decl = (void *)tree->decls[i];
        bool is_fn_decl;

        // Check last indirect type.
        for (tree_t d = (tree_t)decl; d; d = ((struct pointer_s *)d)->indtype)
            is_fn_decl = d->type == FUNCTION;

        if (is_fn_decl)
            continue;

        check_name(decl->name, strict,
            is_global ? global_var_prefix : NULL,
            is_typedef ? typedef_suffix : NULL);
    }
}


static bool is_main(toknum_t name)
{
    token_t *token = &g_tokens[name];
    return (token->end.pos - token->start.pos) == 3 &&
           !memcmp("main", token->start.pos, 4);
}


static void process_fn_def(struct function_def_s *tree)
{
    struct declarator_s *decl = (void *)tree->decl;
    struct specifiers_s *specs = (void *)tree->specs;
    bool with_prefix = g_tokens[specs->storage].kind != KW_STATIC &&
                       !is_main(decl->name);

    check_dirtype(specs->dirtype, allow_short_on_top);
    check_name(decl->name, !allow_short_on_top,
               with_prefix ? global_fn_prefix : NULL, NULL);
}


static void check(void)
{
    iterate_by_type(DECLARATION, process_decl);
    iterate_by_type(FUNCTION_DEF, process_fn_def);
}


REGISTER_RULE(naming, configure, check);
