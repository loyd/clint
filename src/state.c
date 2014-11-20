/*!
 * @brief It provides functions to work with state.
 */

#include <assert.h>
#include <stdlib.h>

#include "clint.h"


char *g_filename = NULL;
char *g_data = NULL;
line_t *g_lines = NULL;
tree_t g_tree = NULL;
token_t *g_tokens = NULL;


void load_file(const char *filename)
{
    assert(filename && filename[0]);
    assert(0 && "Not implemented!");
}


void reset_state(void)
{
    free(g_filename);
    free(g_data);

    if (g_tree)
        dispose_tree(g_tree);

    free_vec(g_lines);
    free_vec(g_tokens);

    g_filename = NULL;
    g_data = NULL;
    g_lines = NULL;
    g_tree = NULL;
    g_tokens = NULL;
}
