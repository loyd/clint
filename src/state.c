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
bool g_cached = false;
token_t *g_tokens = NULL;
error_t *g_errors = NULL;
json_value *g_config = NULL;


void reset_state(void)
{
    free(g_filename);
    free(g_data);

    if (g_tree)
        dispose_tree(g_tree);

    if (g_errors)
        for (unsigned i = 0; i < vec_len(g_errors); ++i)
            free(g_errors[i].message);

    free_vec(g_lines);
    free_vec(g_tokens);
    free_vec(g_errors);

    g_filename = NULL;
    g_data = NULL;
    g_lines = NULL;
    g_tree = NULL;
    g_cached = false;
    g_tokens = NULL;
    g_errors = NULL;
}
