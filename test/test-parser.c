/*!
 * @brief Runner of tests for the lexer.
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clint.h"
#include "helper.h"
#include "tree.h"


static bool check(bool full, const char *input, const char *expected)
{
    static file_t file;
    static char buffer[2048];
    char *actual;

    if (full)
        file.data = input;
    else
    {
        snprintf(buffer, sizeof(buffer), "void t() {%s}", input);
        file.data = buffer;
    }

    parse(&file);

    if (full)
        actual = stringify_tree(file.tree);
    else
    {
        struct transl_unit_s *tu = (void *)file.tree;
        struct function_def_s *fn_def = tu->entities->first->data;
        struct block_s *body = (void *)fn_def->body;

        if (body->entities->first != body->entities->last)
        {
            fprintf(stderr, "Actual has more than 1 entity.\n");
            return false;
        }

        actual = stringify_tree(body->entities->first->data);
    }

    if (strcmp(actual, expected))
    {
        fprintf(stderr, "Actual tree:\n%s\nExpected tree:\n%s",
                actual, expected);
        return false;
    }

    free(actual);
    dispose_tree(file.tree);

    return true;
}


static void parse_tasks(char *data)
{
    static char *group_name, *test_name;
    static char *code, *tree;
    static bool is_full;

#define skip_spaces() while (isspace(*data)) ++data

    /*
        [group name]
        test name*
        =========
        code
        ~~~~~~~~~
        tree
        ~~~~~~~~~
     */
    for (;;)
    {
        skip_spaces();
        if (!*data) break;

        assert(*data == '[' && "Expected group.");
        group_name = ++data;

        data = strchr(data, ']');
        assert(data && "Unfinished group name.");
        *(data++) = '\0';

        group(group_name);

        for (;;)
        {
            skip_spaces();
            if (!*data || *data == '[') break;

            assert(data && "Expected test.");
            test_name = data;

            data = strchr(data, '\n');
            assert(data && "Unfinished test name.");

            if (data[-1] == '*')
            {
                is_full = false;
                data[-1] = '\0';
            }
            else
                is_full = true;

            *(data++) = '\0';

            test(test_name);

            assert(*data == '=' && "Expected separator.");
            data = strchr(data, '\n');
            assert(data && "Expected code section.");
            code = ++data;

            data = strstr(data, "\n~~~");
            assert(data && "Expected separator.");
            *(data++) = '\0';

            data = strchr(data, '\n');
            assert(data && "Expected tree section.");
            tree = ++data;

            data = strstr(data, "\n~~~");
            assert(data && "Expected separator.");
            *(data++) = '\0';
            data = strchr(data, '\n');

            assert(check(is_full, code, tree));
        }
    }
}


void test_parser(void)
{
    size_t size;
    char *data;
    char *pos;

    FILE *fp = fopen("test/test-parser.txt", "r");
    assert(fp);

    // Determine the size.
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read all content from the file.
    data = xmalloc(size + 1);
    fread(data, 1, size, fp);
    fclose(fp);

    data[size] = '\0';
    parse_tasks(data);

    free(data);
}