/*!
 * @brief Iteration over tokens and trees.
 *        Also contains functions for stringify tokens and trees.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clint.h"


enum item_e {TOKEN, TOKENS, NODE, NODES};

typedef bool (*before_t)(const char *prop, enum item_e what, void *raw);
typedef void (*after_t)(const char *prop, enum item_e what, void *raw);


static void iterate_node_inner(void *raw, before_t before, after_t after);

static void iterate(const char *prop, enum item_e what, void *raw,
                    before_t before, after_t after)
{
    assert(raw);

    if (before && !before(prop, what, raw))
        return;

    switch (what)
    {
        case TOKEN:
            break;

        case NODE:
            iterate_node_inner(raw, before, after);
            break;

        case TOKENS:
            for (unsigned i = 0; i < vec_len(raw); ++i)
                iterate(NULL, TOKEN, &((toknum_t *)raw)[i], before, after);
            break;

        case NODES:
            for (unsigned i = 0; i < vec_len(raw); ++i)
                iterate(NULL, NODE, ((tree_t *)raw)[i], before, after);
            break;

        default:
            assert(0);
    }

    if (after)
        after(prop, what, raw);
}


static inline void iterate_node_inner(void *raw, before_t before, after_t after)
{

#define token(prop)                                                           \
    if (tree->prop)                                                           \
        iterate(#prop, TOKEN, &tree->prop, before, after)

#define ITERATE(prop, what)                                                   \
    if (tree->prop)                                                           \
        iterate(#prop, what, tree->prop, before, after)

#define tokens(prop)    ITERATE(prop, TOKENS)
#define node(prop)      ITERATE(prop, NODE)
#define nodes(prop)     ITERATE(prop, NODES)

    switch (((tree_t)raw)->type)
    {
        case TRANSL_UNIT:
        case BLOCK:
        {
            struct transl_unit_s *tree = raw;
            nodes(entities);
            break;
        }
        case DECLARATION:
        {
            struct declaration_s *tree = raw;
            node(specs);
            nodes(decls);
            break;
        }
        case SPECIFIERS:
        {
            struct specifiers_s *tree = raw;
            token(storage);
            token(fnspec);
            tokens(quals);
            node(dirtype);
            break;
        }
        case DECLARATOR:
        {
            struct declarator_s *tree = raw;
            node(indtype);
            token(name);
            node(init);
            node(bitsize);
            break;
        }
        case FUNCTION_DEF:
        {
            struct function_def_s *tree = raw;
            node(specs);
            node(decl);
            nodes(old_decls);
            node(body);
            break;
        }
        case PARAMETER:
        case TYPE_NAME:
        {
            struct parameter_s *tree = raw;
            node(specs);
            node(decl);
            break;
        }
        case ID_TYPE:
        {
            struct id_type_s *tree = raw;
            tokens(names);
            break;
        }
        case STRUCT:
        case UNION:
        {
            struct struct_s *tree = raw;
            token(name);
            nodes(members);
            break;
        }
        case ENUM:
        {
            struct enum_s *tree = raw;
            token(name);
            nodes(values);
            break;
        }
        case ENUMERATOR:
        {
            struct enumerator_s *tree = raw;
            token(name);
            node(value);
            break;
        }
        case POINTER:
        {
            struct pointer_s *tree = raw;
            node(indtype);
            node(specs);
            break;
        }
        case ARRAY:
        {
            struct array_s *tree = raw;
            node(indtype);
            node(dim_specs);
            node(dim);
            break;
        }
        case FUNCTION:
        {
            struct function_s *tree = raw;
            node(indtype);
            nodes(params);
            break;
        }
        case IF:
        case CONDITIONAL:
        {
            struct if_s *tree = raw;
            node(cond);
            node(then_br);
            node(else_br);
            break;
        }
        case SWITCH:
        case WHILE:
        {
            struct switch_s *tree = raw;
            node(cond);
            node(body);
            break;
        }
        case DO_WHILE:
        {
            struct do_while_s *tree = raw;
            node(body);
            node(cond);
            break;
        }
        case FOR:
        {
            struct for_s *tree = raw;
            node(init);
            node(cond);
            node(next);
            node(body);
            break;
        }
        case GOTO:
        {
            struct goto_s *tree = raw;
            token(label);
            break;
        }
        case BREAK:
        case CONTINUE:
        case EMPTY:
            break;
        case RETURN:
        {
            struct return_s *tree = raw;
            node(result);
            break;
        }
        case LABEL:
        {
            struct label_s *tree = raw;
            token(name);
            node(stmt);
            break;
        }
        case DEFAULT:
        {
            struct default_s *tree = raw;
            node(stmt);
            break;
        }
        case CASE:
        {
            struct case_s *tree = raw;
            node(expr);
            node(stmt);
            break;
        }
        case CONSTANT:
        case IDENTIFIER:
        case SPECIAL:
        {
            struct constant_s *tree = raw;
            token(value);
            break;
        }

        case ACCESSOR:
        {
            struct accessor_s *tree = raw;
            node(left);
            token(op);
            token(field);
            break;
        }
        case COMMA:
        {
            struct comma_s *tree = raw;
            nodes(exprs);
            break;
        }
        case CALL:
        {
            struct call_s *tree = raw;
            node(left);
            nodes(args);
            break;
        }
        case CAST:
        {
            struct cast_s *tree = raw;
            node(type_name);
            node(expr);
            break;
        }
        case SUBSCRIPT:
        {
            struct subscript_s *tree = raw;
            node(left);
            node(index);
            break;
        }
        case UNARY:
        {
            struct unary_s *tree = raw;
            token(op);
            node(expr);
            break;
        }
        case BINARY:
        case ASSIGNMENT:
        {
            struct binary_s *tree = raw;
            node(left);
            token(op);
            node(right);
            break;
        }
        case COMP_LITERAL:
        {
            struct comp_literal_s *tree = raw;
            node(type_name);
            nodes(members);
            break;
        }
        case COMP_MEMBER:
        {
            struct comp_member_s *tree = raw;
            nodes(designs);
            node(init);
            break;
        }
    }
}


static struct {
    enum type_e type;
    visitor_t cb;
} iterator;


static bool iterate_by_type_before_cb(const char *prop, enum item_e what,
                                      void *raw)
{
    switch (what)
    {
        case TOKENS:
        case TOKEN:
            return false;

        case NODES:
            return true;

        case NODE:
            return ((tree_t)raw)->type == iterator.type ? iterator.cb(raw)
                                                        : true;
    }
}


void iterate_by_type(enum type_e type, visitor_t cb)
{
    assert(cb);
    iterator.type = type;
    iterator.cb = cb;
    iterate(NULL, NODE, g_tree, iterate_by_type_before_cb, NULL);
}


static const char *stringify_type(enum type_e type)
{
    static const char *words[] = {
        "transl-unit", "empty", "declaration", "specifiers", "declarator",
        "function-def", "parameter", "type-name", "id-type", "struct", "union",
        "enum", "enumerator", "pointer", "array", "function", "block", "if",
        "switch", "while", "do-while", "for", "goto", "break", "continue",
        "return", "label", "default", "case", "constant", "identifier",
        "special", "accessor", "comma", "call", "cast", "conditional",
        "subscript", "unary", "binary", "assignment", "comp-literal",
        "comp-member"
    };

    assert(0 <= type && type < sizeof(words)/sizeof(*words));
    return words[type];
}


// static const char *stringify_kind(enum token_e kind)
// {
//     static const char *words[] = {
// #define XX(kind, word) word,
//     TOK_MAP(XX)
// #undef XX
//     };

//     assert(0 <= kind && kind < sizeof(words)/sizeof(*words));
//     return words[kind];
// }


#define STR_INIT_SIZE 512

static char *str = NULL;
static int str_size;
static int str_len;
static int indent;


__attribute__((format(printf, 1, 2)))
static void push(const char *format, ...)
{
    assert(format);
    assert(str);

    va_list arg;
    int need;
    int avail = str_size - str_len;

    // Try to add to the buffer.
    va_start(arg, format);
    need = vsnprintf(str + str_len, avail, format, arg) + 1;
    va_end(arg);
    assert(need > 0);

    // Expand the buffer if necessary.
    if (avail <= need)
    {
        str_size += need - avail < str_size ? str_size : need;
        str = xrealloc(str, str_size);

        va_start(arg, format);
        need = vsnprintf(str + str_len, need, format, arg) + 1;
        va_end(arg);
        assert(need > 0);
    }

    str_len += need - 1;
}


static void push_indent(void)
{
    push("\n");
    for (int i = 0; i < indent; ++i)
        push("    ");
}


static bool stringify_before_cb(const char *prop, enum item_e what, void *raw)
{
    if (indent > 0 && (prop || what != TOKEN))
        push_indent();

    if (prop)
        push(":%s ", prop);

    ++indent;

    switch (what)
    {
        case TOKEN:
        {
            token_t *tok = &g_tokens[*(toknum_t *)raw];
            int len = tok->end.pos - tok->start.pos;
            push("(%.*s)", len, g_data + tok->start.pos);
            break;
        }

        case NODE:
            push("%s", stringify_type(((tree_t)raw)->type));
            break;

        case NODES:
        case TOKENS:
            push("[");
            break;
    }

    return true;
}


static void stringify_after_cb(const char *prop, enum item_e what, void *raw)
{
    --indent;

    if (what == NODES && vec_len(raw))
        push_indent();

    if (what == NODES || what == TOKENS)
        push("]");
}


char *stringify_tree(void)
{
    char *output;

    indent = 0;

    if (!str)
    {
        str = xmalloc(STR_INIT_SIZE);
        str_size = STR_INIT_SIZE;
        str_len = 0;
    }

    iterate(NULL, NODE, g_tree, stringify_before_cb, stringify_after_cb);

    output = str;
    str = NULL;

    return output;
}


static void dispose_cb(const char *prop, enum item_e what, void *raw)
{
    if (what == NODE)
        free(raw);
    else if (what == NODES || what == TOKENS)
        free_vec(raw);
}


void dispose_tree(tree_t tree)
{
    iterate(NULL, NODE, tree, NULL, dispose_cb);
}
