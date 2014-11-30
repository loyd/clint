#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "clint.h"

// Default settings.
static char *global_var_prefix = NULL;
static char *global_fn_prefix = NULL;
static char *typedef_suffix = NULL;
static char *struct_suffix = NULL;
static char *union_suffix = NULL;
static char *enum_suffix = NULL;
static enum {NONE, UNDER_SCORE} style = NONE;
static int minimum_length = 0;
static bool allow_short_on_top = true;
static bool allow_short_in_loop = true;
static bool allow_short_in_block = true;
static bool disallow_leading_underscore = true;


static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "global-var-prefix", json_string)))
        if (value->u.string.length > 0)
            global_var_prefix = value->u.string.ptr;

    if ((value = json_get(config, "global-fn-prefix", json_string)))
        if (value->u.string.length > 0)
            global_fn_prefix = value->u.string.ptr;

    if ((value = json_get(config, "typedef-suffix", json_string)))
        if (value->u.string.length > 0)
            typedef_suffix = value->u.string.ptr;

    if ((value = json_get(config, "struct-suffix", json_string)))
        if (value->u.string.length > 0)
            struct_suffix = value->u.string.ptr;

    if ((value = json_get(config, "union-suffix", json_string)))
        if (value->u.string.length > 0)
            union_suffix = value->u.string.ptr;

    if ((value = json_get(config, "enum-suffix", json_string)))
        if (value->u.string.length > 0)
            enum_suffix = value->u.string.ptr;

    if ((value = json_get(config, "require-style", json_string)))
        if (!strcmp("none", value->u.string.ptr))
            style = NONE;
        else if (!strcmp("under_score", value->u.string.ptr))
            style = UNDER_SCORE;

    if ((value = json_get(config, "minimum-length", json_integer)))
        if (value->u.integer)
            minimum_length = value->u.integer;

    if ((value = json_get(config, "allow-short-on-top", json_boolean)))
        allow_short_on_top = value->u.boolean;

    if ((value = json_get(config, "allow-short-in-loop", json_boolean)))
        allow_short_in_loop = value->u.boolean;

    if ((value = json_get(config, "allow-short-in-block", json_boolean)))
        allow_short_in_block = value->u.boolean;

    if ((value = json_get(config, "disallow-leading-underscore", json_boolean)))
        disallow_leading_underscore = value->u.boolean;
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
            warn_at(&token->start, "Required \"%s\" prefix", prefix);
    }

    if (suffix)
    {
        slen = strlen(suffix);
        if (memcmp(token->end.pos - slen + 1, suffix, slen))
            warn_at(&token->end, "Required \"%s\" suffix", suffix);
    }

    if (disallow_leading_underscore)
        if (token->start.pos[0] == '_')
            warn_at(&token->start, "Leading underscore is disallowed");

    if (style == UNDER_SCORE)
    {
        for (char *pos = token->start.pos + plen;
             pos <= token->end.pos - slen; ++pos)
            if (!islower(*pos) && *pos != '_')
            {
                warn_at(&token->start, "Required under_score style");
                break;
            }
    }

    if (strict && minimum_length)
        if (token->end.pos - token->start.pos + 1 < minimum_length)
            warn_at(&token->start, "Identifier should be at least %d",
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


static bool process_decl(struct declaration_s *tree)
{
    struct specifiers_s *specs = (void *)tree->specs;
    bool is_global = false;
    bool is_typedef = false;

    if (!specs)
        return false;

    bool strict = !(allow_short_on_top && is_global ||
                    allow_short_in_loop && tree->parent->type == FOR ||
                    allow_short_in_block && !is_global);

    check_dirtype(specs->dirtype, strict);

    if (!tree->decls)
        return false;

    if (tree->parent->type == TRANSL_UNIT && !specs->storage)
        is_global = true;
    else if (g_tokens[specs->storage].kind == KW_TYPEDEF)
        is_typedef = true;

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

    return false;
}


static bool is_main(toknum_t name)
{
    token_t *token = &g_tokens[name];
    return (token->end.pos - token->start.pos) == 3 &&
           !memcmp("main", token->start.pos, 4);
}


static bool process_fn_def(struct function_def_s *tree)
{
    struct declarator_s *decl = (void *)tree->decl;
    struct specifiers_s *specs = (void *)tree->specs;
    bool with_prefix = g_tokens[specs->storage].kind != KW_STATIC &&
                       !is_main(decl->name);

    check_dirtype(specs->dirtype, allow_short_on_top);
    check_name(decl->name, !allow_short_on_top,
               with_prefix ? global_fn_prefix : NULL, NULL);

    return false;
}


static void check(void)
{
    iterate_by_type(DECLARATION, process_decl);
    iterate_by_type(FUNCTION_DEF, process_fn_def);
}


REGISTER_RULE(naming, configure, check);
