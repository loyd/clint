#include <assert.h>
#include <stdbool.h>

#include "clint.h"

static int indent_size = 0;
static char indent_char = '\0';
static unsigned maximum_level = 0;
static bool flat_switch = false;


static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "size", json_none)))
        if (value->type == json_integer)
        {
            if (value->u.integer > 0)
            {
                indent_size = value->u.integer;
                indent_char = ' ';
            }
        }
        else if (value->type == json_string)
            if (value->u.string.ptr[0] == '\t' && !value->u.string.ptr[1])
            {
                indent_size = 1;
                indent_char = '\t';
            }

    if ((value = json_get(config, "maximum-level", json_integer)))
        if (value->u.integer > 0)
            maximum_level = value->u.integer;

    if ((value = json_get(config, "flat-switch", json_boolean)))
        flat_switch = value->u.boolean;
}


static struct {
    unsigned push;
    unsigned pop;
    bool check;
} *lines;

static unsigned *indent_stack;


#define start_of(tree) g_tokens[(tree)->start].start.line
#define end_of(tree) g_tokens[(tree)->end].end.line
#define end_of_prev(tree) g_tokens[(tree)->start - 1].end.line

static bool is_multiline(void *tree)
{
    return start_of((tree_t)tree) != end_of((tree_t)tree);
}


static void mark_check(unsigned line)
{
    lines[line].check = true;
}


static void mark_push(unsigned line)
{
    assert(line < vec_len(g_lines));
    ++lines[line].push;
}


static void mark_pop(unsigned line)
{
    assert(line < vec_len(g_lines));
    ++lines[line].pop;
}


static void mark_children(tree_t *trees)
{
    for (unsigned i = 0; i < vec_len(trees); ++i)
        if (start_of(trees[i]->parent) != start_of(trees[i]))
            mark_check(start_of(trees[i]));
}


static unsigned get_actual_indent(unsigned line)
{
    char *ch = g_lines[line].start;

    while (*ch == indent_char)
        ++ch;

    return ch - g_lines[line].start;
}


static unsigned get_expected_indent(unsigned line, unsigned actual)
{
    assert(vec_len(indent_stack) > 0);
    int pops = lines[line].pop;

    while (pops--)
        --vec_len(indent_stack);

    return indent_stack[vec_len(indent_stack) - 1];
}


static void push_expected_indent(unsigned line, unsigned prev)
{
    unsigned expected = prev + indent_size * lines[line].push;
    vec_push(indent_stack, expected);
}


static void check_like_block(tree_t tree, tree_t *entities)
{
    token_t *lbrace = &g_tokens[tree->start];

    if (!is_multiline(tree))
        return;

    while (lbrace->kind != PN_LBRACE)
        ++lbrace;

    if ((lbrace - 1)->end.line != lbrace->start.line)
        mark_check(lbrace->start.line);

    mark_children(entities);
    mark_check(end_of(tree));

    if (flat_switch && tree->parent->type == SWITCH)
        return;

    mark_push(lbrace->end.line);
    mark_pop(end_of(tree));
}


static tree_t get_deep_case(tree_t tree)
{
    tree_t stmt = tree;

    for (;;)
    {
        stmt = stmt->type == CASE ? ((struct case_s *)tree)->stmt
                                  : ((struct default_s *)tree)->stmt;

        if (stmt->type != CASE && stmt->type != DEFAULT)
            break;

        tree = stmt;
    }

    return tree;
}


static void process_block(struct block_s *tree)
{
    check_like_block((void *)tree, tree->entities);

    if (tree->parent->type == SWITCH && is_multiline(tree))
    {
        bool nested = false;

        for (unsigned i = 0; i < vec_len(tree->entities); ++i)
        {
            tree_t entity = tree->entities[i];

            if (entity->type != CASE && entity->type != DEFAULT)
                continue;

            if (nested)
                mark_pop(start_of(entity));

            nested = lines[start_of(get_deep_case(entity))].push > 0;
        }

        if (nested)
            mark_pop(end_of(tree));
    }
}


static void check_branch(tree_t tree)
{
    if (!tree || end_of_prev(tree) == start_of(tree) || tree->type == BLOCK)
        return;

    mark_check(start_of(tree));
    mark_push(start_of(tree) - 1);
    mark_pop(end_of(tree) + 1);
}


static void process_if(struct if_s *tree)
{
    unsigned cond_end = end_of(tree->cond), else_start;

    if (!is_multiline(tree))
        return;

    if (start_of(tree->cond) == start_of(tree) + 1)
        ++cond_end;

    check_branch(tree->then_br);

    if (!tree->else_br)
        return;

    else_start = end_of_prev(tree->else_br);
    check_branch(tree->else_br);

    if (end_of(tree->then_br) != else_start)
        mark_check(else_start);
}


static void process_for(struct for_s *tree)
{
    if (is_multiline(tree))
        check_branch(tree->body);
}


static void process_while(struct while_s *tree)
{
    if (is_multiline(tree))
        check_branch(tree->body);
}


static void process_struct(struct struct_s *tree)
{
    if (tree->members)
        check_like_block((void *)tree, tree->members);
}


static void process_enum(struct enum_s *tree)
{
    if (tree->values)
        check_like_block((void *)tree, tree->values);
}


static void process_case(tree_t tree)
{
    tree_t stmt = tree->type == CASE ? ((struct case_s *)tree)->stmt
                                     : ((struct default_s *)tree)->stmt;

    mark_check(start_of(tree));
    mark_check(start_of(stmt));

    if (!(start_of(tree) == start_of(stmt) ||
        stmt->type == CASE || stmt->type == DEFAULT || stmt->type == BLOCK))
        mark_push(start_of(tree));
}


static void process_label(struct label_s *tree)
{
    unsigned label_start = start_of(tree);

    lines[label_start + 1].pop += lines[label_start].pop;
    lines[label_start].pop = 0;
    lines[label_start].check = 0;

    if (get_actual_indent(label_start) != 0)
        warn_at(&(location_t){label_start, 0}, "Label must stick to left");
}


static void check(void)
{
    lines = xcalloc(vec_len(g_lines), sizeof(*lines));
    indent_stack = new_vec(unsigned, 8);
    vec_push(indent_stack, 0);

    mark_children(((struct transl_unit_s *)g_tree)->entities);

    iterate_by_type(CASE, process_case);
    iterate_by_type(DEFAULT, process_case);
    iterate_by_type(BLOCK, process_block);
    iterate_by_type(IF, process_if);
    iterate_by_type(FOR, process_for);
    iterate_by_type(WHILE, process_while);
    iterate_by_type(DO_WHILE, process_while);
    iterate_by_type(STRUCT, process_struct);
    iterate_by_type(UNION, process_struct);
    iterate_by_type(ENUM, process_enum);
    iterate_by_type(LABEL, process_label);

    for (unsigned i = 0; i < vec_len(g_lines); ++i)
    {
        unsigned actual = get_actual_indent(i);
        unsigned expected = get_expected_indent(i, actual);

        if (lines[i].check)
        {
            if (actual != expected)
                warn_at(&(location_t){i, actual}, indent_char == '\t' ?
                    "Expected indentation of %u tabs" :
                    "Expected indentation of %u spaces", expected);

            if (maximum_level && actual >= (maximum_level + 1) * indent_size)
                warn_at(&(location_t){i, actual},
                        "Nesting level should not exceed %d", maximum_level);
        }

        if (lines[i].push)
            push_expected_indent(i, expected);
    }

    free_vec(indent_stack);
    free(lines);
}


REGISTER_RULE(indentation, configure, check);
