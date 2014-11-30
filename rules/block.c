#include <stdbool.h>

#include "clint.h"

static bool disallow_empty = false;
static bool disallow_short = false;
static bool disallow_oneline = false;
static bool require_decls_on_top = false;


#define start_of(tree) g_tokens[tree->start].start
#define end_of(tree) g_tokens[tree->end].end

static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "disallow-empty", json_boolean)))
        disallow_empty = value->u.boolean;

    if ((value = json_get(config, "disallow-short", json_boolean)))
        disallow_short = value->u.boolean;

    if ((value = json_get(config, "disallow-oneline", json_boolean)))
        disallow_oneline = value->u.boolean;

    if ((value = json_get(config, "require-decls-on-top", json_boolean)))
        require_decls_on_top = value->u.boolean;
}


static bool find_oneline(tree_t tree)
{
    bool found = false;
    location_t *start = &start_of(tree);

    if (start->line == end_of(tree).line)
        found = true;
    else if (tree->type == DO_WHILE)
    {
        struct do_while_s *loop = (void *)tree;
        found = start->line == start_of(loop->body).line;
    }
    else if (tree->type == IF)
    {
        struct if_s *cond = (void *)tree;
        found = start->line == start_of(cond->then_br).line;
    }

    if (found)
        warn_at(start, "Oneline %s statements are disallowed",
                stringify_type(tree->type));

    return false;
}


static bool process_block(struct block_s *tree)
{
    if (require_decls_on_top)
    {
        unsigned i = 0;
        while (i < vec_len(tree->entities) &&
               tree->entities[i]->type == DECLARATION)
            ++i;

        for (; i < vec_len(tree->entities); ++i)
            if (tree->entities[i]->type == DECLARATION)
                warn_at(&start_of(tree->entities[i]),
                        "Declarations must be on top");
    }

    if (tree->parent->type == FUNCTION_DEF)
        return true;

    if (disallow_empty && vec_len(tree->entities) == 0)
        warn_at(&start_of(tree), "Empty block are disallowed");

    if (disallow_short && vec_len(tree->entities) == 1 &&
        !(tree->parent->type == IF && tree->entities[0]->type == IF))
        warn_at(&start_of(tree), "Short blocks are disallowed");

    return true;
}


static void check(void)
{
    iterate_by_type(BLOCK, process_block);

    if (disallow_oneline)
    {
        iterate_by_type(IF, find_oneline);
        iterate_by_type(FOR, find_oneline);
        iterate_by_type(WHILE, find_oneline);
        iterate_by_type(DO_WHILE, find_oneline);
        iterate_by_type(SWITCH, find_oneline);
    }
}


REGISTER_RULE(block, configure, check);
