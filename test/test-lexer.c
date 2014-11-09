/*!
 * @brief Tests for the lexer.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "clint.h"
#include "helper.h"
#include "tokens.h"


typedef enum token_e T[];

#define check(input, exp) _check(input, exp, sizeof(exp)/sizeof(*exp))

static bool _check(const char *input, enum token_e expected[], int len)
{
    static file_t file;
    static enum token_e actual[20];
    static token_t tok;

    file.data = (char *)input;
    init_lexer(&file);

    int i = 0;
    pull_token(&tok);

    while (tok.kind != TOK_EOF && i < len+1)
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

    return true;
}


GROUP(lexer)
{
    TEST(identifiers)
    {
        assert(check("_", ((T){TOK_IDENTIFIER})));
        assert(check("b", ((T){TOK_IDENTIFIER})));
        assert(check("L", ((T){TOK_IDENTIFIER})));
        assert(check("L01", ((T){TOK_IDENTIFIER})));
        assert(check("\\u1234", ((T){TOK_IDENTIFIER})));
        assert(check("\\U12345678", ((T){TOK_IDENTIFIER})));
        assert(check("\\u123401", ((T){TOK_IDENTIFIER})));
        assert(check("\\U1234567801", ((T){TOK_IDENTIFIER})));
        assert(check("_a", ((T){TOK_IDENTIFIER})));
        assert(check("ba", ((T){TOK_IDENTIFIER})));
        assert(check("La", ((T){TOK_IDENTIFIER})));
        assert(check("La01", ((T){TOK_IDENTIFIER})));
        assert(check("\\u1234a", ((T){TOK_IDENTIFIER})));
        assert(check("\\U12345678a", ((T){TOK_IDENTIFIER})));
        assert(check("_a", ((T){TOK_IDENTIFIER})));
        assert(check("ba", ((T){TOK_IDENTIFIER})));
        assert(check("La", ((T){TOK_IDENTIFIER})));
        assert(check("\\u1234a", ((T){TOK_IDENTIFIER})));
        assert(check("\\U12345678a", ((T){TOK_IDENTIFIER})));
        assert(check("La\\u1234", ((T){TOK_IDENTIFIER})));
        assert(check("La\\u123401", ((T){TOK_IDENTIFIER})));
        assert(check("\\u1234a\\u1234", ((T){TOK_IDENTIFIER})));
        assert(check("\\U12345678a\\u1234", ((T){TOK_IDENTIFIER})));
        assert(check("La\\U12345678", ((T){TOK_IDENTIFIER})));
        assert(check("La\\U1234567801", ((T){TOK_IDENTIFIER})));
        assert(check("\\u1234a\\U12345678", ((T){TOK_IDENTIFIER})));
        assert(check("\\U12345678a\\U12345678", ((T){TOK_IDENTIFIER})));
        assert(check(".el", ((T){PN_PERIOD, TOK_IDENTIFIER})));
    }

#define XX(kind, str)                                                         \
    assert(check(str, ((T){kind})));

    TEST(keywords)
    {
        assert(check("i", ((T){TOK_IDENTIFIER})));
        assert(check("ifo", ((T){TOK_IDENTIFIER})));
        assert(check("elif", ((T){TOK_IDENTIFIER})));

        TOK_KW_MAP(XX)
    }

    TEST(punctuators)
    {
        TOK_PN_MAP(XX)
    }
#undef XX

#define XX(kind, str)                                                         \
    assert(check("#" str, ((T){PN_HASH, kind})));                             \
    assert(check("#  " str, ((T){PN_HASH, kind})));

    TEST(preprocessor keywords)
    {
        assert(check("#i", ((T){PN_HASH, TOK_UNKNOWN})));
        assert(check("#for", ((T){PN_HASH, TOK_UNKNOWN})));
        assert(check("#ifo", ((T){PN_HASH, TOK_UNKNOWN})));
        assert(check("# for", ((T){PN_HASH, TOK_UNKNOWN})));

        TOK_PP_MAP(XX)
    }
#undef XX

    TEST(numeric constants)
    {
        // Integers.
        assert(check("0", ((T){TOK_NUM_CONST})));
        assert(check("0ul", ((T){TOK_NUM_CONST})));
        assert(check("0lu", ((T){TOK_NUM_CONST})));
        assert(check("0LL", ((T){TOK_NUM_CONST})));
        assert(check("0ll", ((T){TOK_NUM_CONST})));
        assert(check("012345", ((T){TOK_NUM_CONST})));
        assert(check("012345LL", ((T){TOK_NUM_CONST})));
        assert(check("012345LLu", ((T){TOK_NUM_CONST})));
        assert(check("012345uLL", ((T){TOK_NUM_CONST})));
        assert(check("012345ull", ((T){TOK_NUM_CONST})));
        assert(check("9", ((T){TOK_NUM_CONST})));
        assert(check("9ul", ((T){TOK_NUM_CONST})));
        assert(check("9lu", ((T){TOK_NUM_CONST})));
        assert(check("9LL", ((T){TOK_NUM_CONST})));
        assert(check("9ll", ((T){TOK_NUM_CONST})));
        assert(check("912345", ((T){TOK_NUM_CONST})));
        assert(check("912345LL", ((T){TOK_NUM_CONST})));
        assert(check("912345LLu", ((T){TOK_NUM_CONST})));
        assert(check("912345uLL", ((T){TOK_NUM_CONST})));
        assert(check("912345ull", ((T){TOK_NUM_CONST})));
        assert(check("9", ((T){TOK_NUM_CONST})));
        assert(check("9ul", ((T){TOK_NUM_CONST})));
        assert(check("9lu", ((T){TOK_NUM_CONST})));
        assert(check("9LL", ((T){TOK_NUM_CONST})));
        assert(check("9ll", ((T){TOK_NUM_CONST})));
        assert(check("0xb12345", ((T){TOK_NUM_CONST})));
        assert(check("0Xb12345LL", ((T){TOK_NUM_CONST})));
        assert(check("0xB12345LLu", ((T){TOK_NUM_CONST})));
        assert(check("0Xb12345uLL", ((T){TOK_NUM_CONST})));
        assert(check("0xB12345ull", ((T){TOK_NUM_CONST})));

        // Floatings.
        assert(check("0.f", ((T){TOK_NUM_CONST})));
        assert(check("0.F", ((T){TOK_NUM_CONST})));
        assert(check("0.l", ((T){TOK_NUM_CONST})));
        assert(check("0.L", ((T){TOK_NUM_CONST})));
        assert(check(".1f", ((T){TOK_NUM_CONST})));
        assert(check(".2L", ((T){TOK_NUM_CONST})));
        assert(check("0.1", ((T){TOK_NUM_CONST})));
        assert(check("0.1f", ((T){TOK_NUM_CONST})));
        assert(check("0.1L", ((T){TOK_NUM_CONST})));
        assert(check("0.1e-2", ((T){TOK_NUM_CONST})));
        assert(check("0.1e+2", ((T){TOK_NUM_CONST})));
        assert(check("0.1e2", ((T){TOK_NUM_CONST})));
        assert(check("0.1e+2f", ((T){TOK_NUM_CONST})));
        assert(check("0.1E-2L", ((T){TOK_NUM_CONST})));
        assert(check("0xf.f", ((T){TOK_NUM_CONST})));
        assert(check("0xf.", ((T){TOK_NUM_CONST})));
        assert(check("0xF.L", ((T){TOK_NUM_CONST})));
        assert(check("0xf.f", ((T){TOK_NUM_CONST})));
        assert(check("0xF.L", ((T){TOK_NUM_CONST})));
        assert(check("0xf.ap-3", ((T){TOK_NUM_CONST})));
        assert(check("0xf.ap+3", ((T){TOK_NUM_CONST})));
        assert(check("0xf.ap3", ((T){TOK_NUM_CONST})));
        assert(check("0xF.aP+3L", ((T){TOK_NUM_CONST})));
        assert(check("0xf.aP-3f", ((T){TOK_NUM_CONST})));
        assert(check("0xF.aP+3L", ((T){TOK_NUM_CONST})));
        assert(check("0xfe+", ((T){TOK_NUM_CONST, PN_PLUS})));
    }

    TEST(character constants)
    {
        assert(check("''", ((T){TOK_CHAR_CONST})));
        assert(check("L''", ((T){TOK_CHAR_CONST})));
        assert(check("'n'", ((T){TOK_CHAR_CONST})));
        assert(check("L'n'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\\\'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\''", ((T){TOK_CHAR_CONST})));
        assert(check("'\\n'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\0'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\01'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\012'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\x1'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\x12'", ((T){TOK_CHAR_CONST})));
        assert(check("'\\u1234'", ((T){TOK_CHAR_CONST})));
        assert(check("'", ((T){TOK_UNKNOWN})));
        assert(check("'\n", ((T){TOK_UNKNOWN})));
        assert(check("'\\u1234", ((T){TOK_UNKNOWN})));
        assert(check("'\\u1234\n", ((T){TOK_UNKNOWN})));
    }

    TEST(strings)
    {
        assert(check("\"\"", ((T){TOK_STRING})));
        assert(check("L\"\"", ((T){TOK_STRING})));
        assert(check("\"n\"", ((T){TOK_STRING})));
        assert(check("L\"n\"", ((T){TOK_STRING})));
        assert(check("\"\\\\\"", ((T){TOK_STRING})));
        assert(check("\"\\n\"", ((T){TOK_STRING})));
        assert(check("\"\\0\"", ((T){TOK_STRING})));
        assert(check("\"\\01\"", ((T){TOK_STRING})));
        assert(check("\"\\012\"", ((T){TOK_STRING})));
        assert(check("\"\\x1\"", ((T){TOK_STRING})));
        assert(check("\"\\x12\"", ((T){TOK_STRING})));
        assert(check("\"\\u1234\"", ((T){TOK_STRING})));
        assert(check("\"", ((T){TOK_UNKNOWN})));
        assert(check("\"\n", ((T){TOK_UNKNOWN})));
        assert(check("\"\\u1234", ((T){TOK_UNKNOWN})));
        assert(check("\"\\u1234\n", ((T){TOK_UNKNOWN})));
    }

    TEST(header names)
    {
        assert(check("#include <test.h>",
                     ((T){PN_HASH, PP_INCLUDE, TOK_HEADER_NAME})));
        assert(check("#include \"test.h\"",
                     ((T){PN_HASH, PP_INCLUDE, TOK_HEADER_NAME})));
        assert(check("#include <test.h",
                     ((T){PN_HASH, PP_INCLUDE, TOK_UNKNOWN})));
        assert(check("#include \"test.h",
                     ((T){PN_HASH, PP_INCLUDE, TOK_UNKNOWN})));
    }

    TEST(comments)
    {
        assert(check("// test comment", ((T){TOK_COMMENT})));
        assert(check("//", ((T){TOK_COMMENT})));
        assert(check("//\n", ((T){TOK_COMMENT})));
        assert(check("/**/", ((T){TOK_COMMENT})));
        assert(check("/* test comment */", ((T){TOK_COMMENT})));
        assert(check("/** test \n comment */", ((T){TOK_COMMENT})));
        assert(check("/*/", ((T){TOK_UNKNOWN})));
        assert(check("/*", ((T){TOK_UNKNOWN})));
        assert(check("/*\n", ((T){TOK_UNKNOWN})));
        assert(check("/* test\n", ((T){TOK_UNKNOWN})));
    }
}
