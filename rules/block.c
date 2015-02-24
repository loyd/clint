#include <stdbool.h>
#include <string.h>

#include "clint.h"

static bool disallow_empty;
static bool disallow_short;
static bool disallow_oneline;
static bool require_decls_on_top;
static char **allow_before_decls;


#define start_of(tree) g_tokens[tree->start].start
#define end_of(tree) g_tokens[tree->end].end

static void configure(void)
{
    disallow_empty = cfg_boolean("disallow-empty");
    disallow_short = cfg_boolean("disallow-short");
    disallow_oneline = cfg_boolean("disallow-oneline");
    require_decls_on_top = cfg_boolean("require-decls-on-top");
    allow_before_decls = cfg_strings("allow-before-decls");
}


static void find_oneline(tree_t tree)
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
        add_warn_at(*start, "Oneline %s statements are disallowed",
                    stringify_type(tree->type));
}


static bool in_decl_area(tree_t tree)
{
    if (tree->type == DECLARATION)
        return true;

    if (tree->type == CALL && allow_before_decls)
    {
        token_t *name = &g_tokens[tree->start];
        unsigned len = name->end.pos - name->start.pos + 1;

        for (unsigned i = 0; i < vec_len(allow_before_decls); ++i)
            if (!strncmp(allow_before_decls[i], name->start.pos, len))
                return true;
    }

    return false;
}


static void process_block(struct block_s *tree)
{
    if (require_decls_on_top)
    {
        unsigned i = 0;
        while (i < vec_len(tree->entities) && in_decl_area(tree->entities[i]))
            ++i;

        for (; i < vec_len(tree->entities); ++i)
            if (tree->entities[i]->type == DECLARATION)
                add_warn_at(start_of(tree->entities[i]),
                            "Declarations must be on top");
    }

    if (tree->parent->type == FUNCTION_DEF)
        return;

    if (disallow_empty && vec_len(tree->entities) == 0)
        add_warn_at(start_of(tree), "Empty block are disallowed");

    if (disallow_short && vec_len(tree->entities) == 1 &&
        tree->parent->type != SWITCH &&
        !(tree->parent->type == IF && tree->entities[0]->type == IF))
        add_warn_at(start_of(tree), "Short blocks are disallowed");
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
