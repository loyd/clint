/*!
 * @brief Tests for the lexer.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "clint.h"
#include "helper.h"
#include "tokens.h"


typedef enum token_e v_t[];

#define check(input, exp) check(input, exp, sizeof(exp) / sizeof(*exp))

static bool (check)(const char *input, enum token_e expected[], int len)
{
    static char buffer[1024];
    static enum token_e actual[20];
    static token_t tok;
    int i = 0;

    g_data = strncpy(buffer, input, sizeof(buffer));
    init_lexer();

    pull_token(&tok);

    while (tok.kind != TOK_EOF && i < len + 1)
    {
        actual[i++] = tok.kind;
        pull_token(&tok);
    }

    if (i < len)
    {
        fprintf(stderr, "Received %d tokens, but expected %d.\n", i, len);
        return false;
    }

    if (i > len)
    {
        fprintf(stderr, "Received more tokens than expected (%d).\n", len);
        return false;
    }

    if (memcmp(actual, expected, len * sizeof(*actual)) != 0)
    {
        for (int j = 0; j < len; ++j)
            if (actual[j] != expected[j])
                fprintf(stderr, "actual[%d] != expected[%d].\n", j, j);

        return false;
    }

    g_data = NULL;
    reset_state();
    return true;
}


void test_lexer(void)
{
    group("lexer");

    test("identifiers");
    {
        assert(check("_", ((v_t){TOK_IDENTIFIER})));
        assert(check("b", ((v_t){TOK_IDENTIFIER})));
        assert(check("L", ((v_t){TOK_IDENTIFIER})));
        assert(check("L01", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\u1234", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\U12345678", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\u123401", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\U1234567801", ((v_t){TOK_IDENTIFIER})));
        assert(check("_a", ((v_t){TOK_IDENTIFIER})));
        assert(check("ba", ((v_t){TOK_IDENTIFIER})));
        assert(check("La", ((v_t){TOK_IDENTIFIER})));
        assert(check("La01", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\u1234a", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\U12345678a", ((v_t){TOK_IDENTIFIER})));
        assert(check("_a", ((v_t){TOK_IDENTIFIER})));
        assert(check("ba", ((v_t){TOK_IDENTIFIER})));
        assert(check("La", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\u1234a", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\U12345678a", ((v_t){TOK_IDENTIFIER})));
        assert(check("La\\u1234", ((v_t){TOK_IDENTIFIER})));
        assert(check("La\\u123401", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\u1234a\\u1234", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\U12345678a\\u1234", ((v_t){TOK_IDENTIFIER})));
        assert(check("La\\U12345678", ((v_t){TOK_IDENTIFIER})));
        assert(check("La\\U1234567801", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\u1234a\\U12345678", ((v_t){TOK_IDENTIFIER})));
        assert(check("\\U12345678a\\U12345678", ((v_t){TOK_IDENTIFIER})));
        assert(check(".el", ((v_t){PN_PERIOD, TOK_IDENTIFIER})));
    }

#define XX(kind, str)                                                         \
    assert(check(str, ((v_t){kind})));

    test("keywords");
    {
        assert(check("i", ((v_t){TOK_IDENTIFIER})));
        assert(check("i{", ((v_t){TOK_IDENTIFIER, PN_LBRACE})));
        assert(check("ifo", ((v_t){TOK_IDENTIFIER})));
        assert(check("elif", ((v_t){TOK_IDENTIFIER})));

        TOK_KW_MAP(XX)
    }

    test("punctuators");
    {
        assert(check("..!", ((v_t){PN_PERIOD, PN_PERIOD, PN_EXCLAIM})));
        TOK_PN_MAP(XX)
    }
#undef XX

#define XX(kind, str)                                                         \
    assert(check("#" str, ((v_t){PN_HASH, kind})));                           \
    assert(check("#  " str, ((v_t){PN_HASH, kind})));

    test("preprocessor keywords");
    {
        assert(check("#i", ((v_t){PN_HASH, TOK_UNKNOWN})));
        assert(check("#i{", ((v_t){PN_HASH, TOK_UNKNOWN, PN_LBRACE})));
        assert(check("#ifo", ((v_t){PN_HASH, TOK_UNKNOWN})));
        assert(check("#for", ((v_t){PN_HASH, TOK_UNKNOWN})));
        assert(check("# for", ((v_t){PN_HASH, TOK_UNKNOWN})));

        TOK_PP_MAP(XX)
    }
#undef XX

    test("numeric constants");
    {
        // Integers.
        assert(check("0", ((v_t){TOK_NUM_CONST})));
        assert(check("0ul", ((v_t){TOK_NUM_CONST})));
        assert(check("0lu", ((v_t){TOK_NUM_CONST})));
        assert(check("0LL", ((v_t){TOK_NUM_CONST})));
        assert(check("0ll", ((v_t){TOK_NUM_CONST})));
        assert(check("012345", ((v_t){TOK_NUM_CONST})));
        assert(check("012345LL", ((v_t){TOK_NUM_CONST})));
        assert(check("012345LLu", ((v_t){TOK_NUM_CONST})));
        assert(check("012345uLL", ((v_t){TOK_NUM_CONST})));
        assert(check("012345ull", ((v_t){TOK_NUM_CONST})));
        assert(check("9", ((v_t){TOK_NUM_CONST})));
        assert(check("9ul", ((v_t){TOK_NUM_CONST})));
        assert(check("9lu", ((v_t){TOK_NUM_CONST})));
        assert(check("9LL", ((v_t){TOK_NUM_CONST})));
        assert(check("9ll", ((v_t){TOK_NUM_CONST})));
        assert(check("912345", ((v_t){TOK_NUM_CONST})));
        assert(check("912345LL", ((v_t){TOK_NUM_CONST})));
        assert(check("912345LLu", ((v_t){TOK_NUM_CONST})));
        assert(check("912345uLL", ((v_t){TOK_NUM_CONST})));
        assert(check("912345ull", ((v_t){TOK_NUM_CONST})));
        assert(check("9", ((v_t){TOK_NUM_CONST})));
        assert(check("9ul", ((v_t){TOK_NUM_CONST})));
        assert(check("9lu", ((v_t){TOK_NUM_CONST})));
        assert(check("9LL", ((v_t){TOK_NUM_CONST})));
        assert(check("9ll", ((v_t){TOK_NUM_CONST})));
        assert(check("0xb12345", ((v_t){TOK_NUM_CONST})));
        assert(check("0Xb12345LL", ((v_t){TOK_NUM_CONST})));
        assert(check("0xB12345LLu", ((v_t){TOK_NUM_CONST})));
        assert(check("0Xb12345uLL", ((v_t){TOK_NUM_CONST})));
        assert(check("0xB12345ull", ((v_t){TOK_NUM_CONST})));

        // Floatings.
        assert(check("0.f", ((v_t){TOK_NUM_CONST})));
        assert(check("0.F", ((v_t){TOK_NUM_CONST})));
        assert(check("0.l", ((v_t){TOK_NUM_CONST})));
        assert(check("0.L", ((v_t){TOK_NUM_CONST})));
        assert(check(".1f", ((v_t){TOK_NUM_CONST})));
        assert(check(".2L", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1f", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1L", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1e-2", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1e+2", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1e2", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1e+2f", ((v_t){TOK_NUM_CONST})));
        assert(check("0.1E-2L", ((v_t){TOK_NUM_CONST})));
        assert(check("0xf.f", ((v_t){TOK_NUM_CONST})));
        assert(check("0xf.", ((v_t){TOK_NUM_CONST})));
        assert(check("0xF.L", ((v_t){TOK_NUM_CONST})));
        assert(check("0xf.f", ((v_t){TOK_NUM_CONST})));
        assert(check("0xF.L", ((v_t){TOK_NUM_CONST})));
        assert(check("0xf.ap-3", ((v_t){TOK_NUM_CONST})));
        assert(check("0xf.ap+3", ((v_t){TOK_NUM_CONST})));
        assert(check("0xf.ap3", ((v_t){TOK_NUM_CONST})));
        assert(check("0xF.aP+3L", ((v_t){TOK_NUM_CONST})));
        assert(check("0xf.aP-3f", ((v_t){TOK_NUM_CONST})));
        assert(check("0xF.aP+3L", ((v_t){TOK_NUM_CONST})));
        assert(check("0xfe+", ((v_t){TOK_NUM_CONST, PN_PLUS})));
    }

    test("character constants");
    {
        assert(check("''", ((v_t){TOK_CHAR_CONST})));
        assert(check("L''", ((v_t){TOK_CHAR_CONST})));
        assert(check("'n'", ((v_t){TOK_CHAR_CONST})));
        assert(check("L'n'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\\\'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\''", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\n'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\0'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\01'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\012'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\x1'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\x12'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\u1234'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'", ((v_t){TOK_UNKNOWN})));
        assert(check("'\n", ((v_t){TOK_UNKNOWN})));
        assert(check("'\\u1234", ((v_t){TOK_UNKNOWN})));
        assert(check("'\\u1234\n", ((v_t){TOK_UNKNOWN})));
    }

    test("strings");
    {
        assert(check("\"\"", ((v_t){TOK_STRING})));
        assert(check("L\"\"", ((v_t){TOK_STRING})));
        assert(check("\"n\"", ((v_t){TOK_STRING})));
        assert(check("L\"n\"", ((v_t){TOK_STRING})));
        assert(check("\"\\\\\"", ((v_t){TOK_STRING})));
        assert(check("\"\\n\"", ((v_t){TOK_STRING})));
        assert(check("\"\\0\"", ((v_t){TOK_STRING})));
        assert(check("\"\\01\"", ((v_t){TOK_STRING})));
        assert(check("\"\\012\"", ((v_t){TOK_STRING})));
        assert(check("\"\\x1\"", ((v_t){TOK_STRING})));
        assert(check("\"\\x12\"", ((v_t){TOK_STRING})));
        assert(check("\"\\u1234\"", ((v_t){TOK_STRING})));
        assert(check("\"", ((v_t){TOK_UNKNOWN})));
        assert(check("\"\n", ((v_t){TOK_UNKNOWN})));
        assert(check("\"\\u1234", ((v_t){TOK_UNKNOWN})));
        assert(check("\"\\u1234\n", ((v_t){TOK_UNKNOWN})));
    }

    test("header names");
    {
        assert(check("#include <test.h>",
                     ((v_t){PN_HASH, PP_INCLUDE, TOK_HEADER_NAME})));
        assert(check("#include \"test.h\"",
                     ((v_t){PN_HASH, PP_INCLUDE, TOK_HEADER_NAME})));
        assert(check("#include <test.h",
                     ((v_t){PN_HASH, PP_INCLUDE, TOK_UNKNOWN})));
        assert(check("#include \"test.h",
                     ((v_t){PN_HASH, PP_INCLUDE, TOK_UNKNOWN})));
    }

    test("comments");
    {
        assert(check("// test comment", ((v_t){TOK_COMMENT})));
        assert(check("//", ((v_t){TOK_COMMENT})));
        assert(check("//\n", ((v_t){TOK_COMMENT})));
        assert(check("/**/", ((v_t){TOK_COMMENT})));
        assert(check("/* test comment */", ((v_t){TOK_COMMENT})));
        assert(check("/** test \n comment */", ((v_t){TOK_COMMENT})));
        assert(check("/*/", ((v_t){TOK_UNKNOWN})));
        assert(check("/*", ((v_t){TOK_UNKNOWN})));
        assert(check("/*\n", ((v_t){TOK_UNKNOWN})));
        assert(check("/* test\n", ((v_t){TOK_UNKNOWN})));
    }

    test("backslash + newline");
    {
        assert(check("a\\\nb", ((v_t){TOK_IDENTIFIER})));
        assert(check("a\\\rb", ((v_t){TOK_IDENTIFIER})));
        assert(check("a\\\r\nb", ((v_t){TOK_IDENTIFIER})));
        assert(check("a\\  \nb", ((v_t){TOK_IDENTIFIER})));
        assert(check("a\\  \r\nb", ((v_t){TOK_IDENTIFIER})));
        assert(check("\"a\\\nb\"", ((v_t){TOK_STRING})));
        assert(check("\"a\\\r\nb\"", ((v_t){TOK_STRING})));
        assert(check("'\\\\\n0'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\\\\r\n0'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\\\  \n0'", ((v_t){TOK_CHAR_CONST})));
        assert(check("'\\\\  \r\n0'", ((v_t){TOK_CHAR_CONST})));
        assert(check("1000  \\   \n + \\\n 20",
            ((v_t){TOK_NUM_CONST, PN_PLUS, TOK_NUM_CONST})));
        assert(check("1000  \\   \r\n + \\\r\n 20",
            ((v_t){TOK_NUM_CONST, PN_PLUS, TOK_NUM_CONST})));
        assert(check("1+ \\\n\\\n\\\n 2",
            ((v_t){TOK_NUM_CONST, PN_PLUS, TOK_NUM_CONST})));
        assert(check("1+ \\\r\n\\\r\n\\\r\n 2",
            ((v_t){TOK_NUM_CONST, PN_PLUS, TOK_NUM_CONST})));
    }
}
