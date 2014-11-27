#include <stdbool.h>

#include "clint.h"

static bool disallow_empty = false;
static bool disallow_short = false;
static bool disallow_oneline_if = false;
static bool require_decls_on_top = false;


static void configure(json_value *config)
{
    json_value *value;

    if ((value = json_get(config, "disallow-empty", json_boolean)))
        disallow_empty = value->u.boolean;

    if ((value = json_get(config, "disallow-short", json_boolean)))
        disallow_short = value->u.boolean;

    if ((value = json_get(config, "disallow-oneline-if", json_boolean)))
        disallow_oneline_if = value->u.boolean;

    if ((value = json_get(config, "require-decls-on-top", json_boolean)))
        require_decls_on_top = value->u.boolean;
}


static bool find_oneline_if(struct if_s *tree)
{
    location_t *rparen, *branch;
    rparen = &g_tokens[tree->then_br->start - 1].end;
    branch = &g_tokens[tree->then_br->type != BLOCK ? tree->then_br->start
                                                    : tree->then_br->end].start;

    if (rparen->line == branch->line)
        warn_at(branch, "Oneline if statements are disallowed");

    return true;
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
                warn_at(&g_tokens[tree->entities[i]->start].start,
                        "Declarations must be on top");
    }

    if (tree->parent->type == FUNCTION_DEF)
        return true;

    if (disallow_empty && vec_len(tree->entities) == 0)
        warn_at(&g_tokens[tree->start].end, "Empty block are disallowed");

    if (disallow_short && vec_len(tree->entities) == 1 &&
        !(tree->parent->type == IF && tree->entities[0]->type == IF))
        warn_at(&g_tokens[tree->start].end, "Short blocks are disallowed");

    return true;
}


static void check(void)
{
    iterate_by_type(BLOCK, process_block);

    if (disallow_oneline_if)
        iterate_by_type(IF, find_oneline_if);
}


REGISTER_RULE(block, configure, check);
