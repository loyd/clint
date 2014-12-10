/*!
 * @brief It contains functions to perform lexical analysis.
 *        The lexer allows many inaccuracies in tokens.
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "clint.h"


/*!
 * @name The lexer state
 * The global state of the lexer. It's reset before process another file.
 */
//!@{
static char *ch;

static bool parsing_header_name;
static bool parsing_pp_directive;
//!@}


#define error(...)                                                            \
    (add_error(vec_len(g_lines) - 1, ch - g_lines[vec_len(g_lines) - 1].start,\
               __VA_ARGS__), false)


void init_lexer(void)
{
    assert(g_data);
    assert(!g_lines);

    g_lines = new_vec(line_t, 128);
    vec_push(g_lines, ((line_t){g_data, 0, false}));

    ch = g_data;
    parsing_header_name = false;
    parsing_pp_directive = false;
}


/**
 * Well, there is no need to use a perfect hash function (e.g. by `gperf`),
 * because it's not a bottleneck.
 *
 * Instead, we have sorted search tables for both case (for keywords and
 * preprocessor keywords) and use a binary search.
 */
//!@{
struct extstr_s {
    const char *data;
    int len;
};


static int comparator(struct extstr_s *key, struct extstr_s *entry)
{
    int len = key->len > entry->len ? entry->len + 1
            : key->len < entry->len ? key->len + 1
                                    : key->len;

    return memcmp(key->data, entry->data, len);
}


static inline enum token_e find_kw(const char *word, int len)
{
    static struct extstr_s table[] = {
#define XX(kind, word) {word, sizeof(word)-1},
        TOK_KW_MAP(XX)
#undef XX
    };

    static struct extstr_s key;
    struct extstr_s *res;

    key.data = word;
    key.len = len;

    res = bsearch((void *)&key,
        table, sizeof(table) / sizeof(*table), sizeof(*table),
        (int (*)(const void *, const void *))comparator);

    return res ? (res - table) + KW_BOOL : TOK_UNKNOWN;
}


static inline enum token_e find_pp(const char *word, int len)
{
    static struct extstr_s table[] = {
#define XX(kind, word) {word, sizeof(word)-1},
        TOK_PP_MAP(XX)
#undef XX
    };

    static struct extstr_s key;
    struct extstr_s *res;

    key.data = word;
    key.len = len;

    res = bsearch((void *)&key,
        table, sizeof(table) / sizeof(*table), sizeof(*table),
        (int (*)(const void *, const void *))comparator);

    return res ? (res - table) + PP_DEFINE : TOK_UNKNOWN;
}
//!@}


static inline unsigned get_column(const char *c)
{
    return c - g_lines[vec_len(g_lines) - 1].start;
}


static int is_nel(const char *c)
{
    if (*c == '\n')
        return 1;

    if (*c == '\r')
        return c[1] == '\n' ? 2 : 1;

    return 0;
}

static void eat(int num)
{
    assert(num > 0);
    int nel;

    if ((nel = is_nel(ch)))
    {
        g_lines[vec_len(g_lines) - 1].length = get_column(ch);
        vec_push(g_lines, ((line_t){ch + nel, 0, false}));

        if (nel > 1)
            ++ch;
    }

    // Frequent case.
    if (num == 1 && ch[1] != '\\')
    {
        ++ch;
        return;
    }

    // Common case.
    do
        /* Check backslash + newline. This approach doesn't cover all cases,
         * but it's sufficient for literals, macros and identifiers.
         * Therefore this cannot affect any real program.
         */
        while (*++ch == '\\')
        {
            while (isspace(ch[1]) && !is_nel(ch + 1))
                ++ch;

            if (isspace(ch[1]))
                ++ch;

            if (!(nel = is_nel(ch)))
                break;

            g_lines[vec_len(g_lines) - 1].dangling = true;
            g_lines[vec_len(g_lines) - 1].length = get_column(ch);
            vec_push(g_lines, ((line_t){ch + nel, 0, false}));

            if (nel > 1)
                ++ch;
        }
    while (--num);
}


static inline void skip_spaces(void)
{
    while (isspace(*ch))
        eat(1);
}


/*!
 * C99 6.4.4.1: Integer constants.
 * C99 6.4.4.2: Floating constants.
 *
 *   decimal integer: [1-9]{D}*{IS}?           (1)
 *   octal integer: 0[0-7]*{IS}?               (2)
 *   hexa integer: 0[xX]{H}+{IS}?              (3)
 *   decimal floating: {D}+{E}{FS}?            (4)
 *                     {D}*\.{D}+{E}?{FS}?     (5)
 *                     {D}+\.{D}*{E}?{FS}?     (6)
 *   hexa floating: 0[xX]{H}+{P}{FS}?          (7)
 *                  0[xX]{H}*\.{H}+{P}?{FS}?   (8)
 *                  0[xX]{H}+\.{H}*{P}?{FS}?   (9)
 * , where
 *   D: [0-9]
 *   H: [a-fA-F0-9]
 *   IS: ([uU]|[uU]?(l|L|ll|LL)|(l|L|ll|LL)[uU])
 *   FS: [fFlL]
 *   P: ([Pp][+-]?{D}+)
 *   E: ([Ee][+-]?{D}+)
 */
static bool numeric_const(token_t *token)
{
    assert(token);
    assert(isdigit(*ch) || *ch == '.');

    bool is_float = false;

    while (isxdigit(*ch))
        eat(1);
    if (tolower(*ch) == 'x')
        eat(1);
    while (isxdigit(*ch))
        eat(1);

    if (*ch == '.')
    {
        is_float = true;
        eat(1);
    }

    while (isxdigit(*ch))
        eat(1);
    if (tolower(*ch) == 'p')
        eat(1);

    if (is_float && (tolower(ch[-1]) == 'e' || tolower(ch[-1]) == 'p'))
    {
        if (*ch == '+' || *ch == '-')
            eat(1);
        while (isdigit(*ch))
            eat(1);
    }

    while (isalpha(*ch))
        eat(1);

    token->kind = TOK_NUM_CONST;
    return true;
}


/*!
 * C99 6.4.4.4: Character constants.
 */
static bool char_const(token_t *token)
{
    assert(token);
    assert(*ch == '\'' || *ch == 'L' && ch[1] == '\'');

    eat(*ch == 'L' ? 2 : 1);
    while (*ch && !is_nel(ch) && *ch != '\'')
    {
        if (*ch == '\\')
            eat(1);
        if (*ch)
            eat(1);
    }

    if (*ch != '\'')
        return error("Unexpected %s while parsing character constant",
                     *ch ? "newline" : "EOF");

    eat(1);

    token->kind = TOK_CHAR_CONST;
    return true;
}


/*!
 * C99 6.4.5: String literals.
 */
static bool string_literal(token_t *token)
{
    assert(token);
    assert(*ch == '"' || *ch == 'L' && ch[1] == '"');

    eat(*ch == 'L' ? 2 : 1);
    while (*ch && !is_nel(ch) && *ch != '"')
    {
        if (*ch == '\\')
            eat(1);
        if (*ch)
            eat(1);
    }

    if (*ch != '"')
        return error("Unexpected %s while parsing string literal",
                     *ch ? "newline" : "EOF");

    eat(1);

    token->kind = TOK_STRING;
    return true;
}


/*!
 * C99 6.4.3: Universal character names.
 */
static bool check_ucn(void)
{
    int digits;

    if (!(*ch == '\\' && tolower(ch[1]) == 'u'))
        return false;

    digits = ch[1] == 'u' ? 4 : 8;
    for (int i = 0; i < digits; ++i)
        if (!isxdigit(ch[i + 2]))
            return false;

    return true;
}


/*!
 * C99 6.4.1: Keywords.
 * C99 6.4.2: Identifiers.
 *
 *   identifier: ([_a-zA-Z]|{U})(\w|{U})*
 * , where
 *   H: [a-fA-F0-9]{4}
 *   U: (\\u{H})|(\\U{H}{H})
 */
static bool identifier(token_t *token)
{
    assert(token);
    assert(isalpha(*ch) || *ch == '_' || check_ucn());

    const char *start = ch;

    do
        eat(*ch == '\\' ? (ch[1] == 'u' ? 6 : 10) : 1);
    while (isalnum(*ch) || *ch == '_' || check_ucn());

    if (parsing_pp_directive)
    {
        token->kind = find_pp(start, ch - start);

        if (token->kind == PP_INCLUDE)
            parsing_header_name = true;

        parsing_pp_directive = false;
    }
    else
    {
        token->kind = find_kw(start, ch - start);

        if (token->kind == TOK_UNKNOWN)
            token->kind = TOK_IDENTIFIER;
    }

    return true;
}


/*!
 * C99 6.4.6: Punctuators.
 */
static bool punctuator(token_t *token)
{
    //#TODO: support for digraphs and trigraphs.
    assert(token);

    static const char *puncts[] = {
#define XX(kind, word) word,
    TOK_PN_MAP(XX)
#undef XX
    };

    enum token_e kind;

    switch (*ch)
    {
        case '[': kind = PN_LSQUARE; break;
        case ']': kind = PN_RSQUARE; break;
        case '(': kind = PN_LPAREN; break;
        case ')': kind = PN_RPAREN; break;
        case '{': kind = PN_LBRACE; break;
        case '}': kind = PN_RBRACE; break;
        case '~': kind = PN_TILDE; break;
        case '?': kind = PN_QUESTION; break;
        case ':': kind = PN_COLON; break;
        case ';': kind = PN_SEMI; break;
        case ',': kind = PN_COMMA; break;

        case '!': kind = ch[1] == '=' ? PN_EXCLAIMEQ : PN_EXCLAIM; break;
        case '/': kind = ch[1] == '=' ? PN_SLASHEQ : PN_SLASH; break;
        case '%': kind = ch[1] == '=' ? PN_PERCENTEQ : PN_PERCENT; break;
        case '^': kind = ch[1] == '=' ? PN_CARETEQ : PN_CARET; break;
        case '=': kind = ch[1] == '=' ? PN_EQEQ : PN_EQ; break;
        case '#': kind = ch[1] == '#' ? PN_HASHHASH : PN_HASH; break;

        case '.':
            kind = ch[1] == '.' && ch[2] == '.' ? PN_ELLIPSIS : PN_PERIOD;
            break;

        case '&':
            kind = ch[1] == '&' ? PN_AMPAMP
                 : ch[1] == '=' ? PN_AMPEQ
                                : PN_AMP;
            break;

        case '*':
            kind = ch[1] == '=' ? PN_STAREQ
                                : PN_STAR;
            break;

        case '+':
            kind = ch[1] == '+' ? PN_PLUSPLUS
                 : ch[1] == '=' ? PN_PLUSEQ
                                : PN_PLUS;
            break;

        case '-':
            kind = ch[1] == '>' ? PN_ARROW
                 : ch[1] == '-' ? PN_MINUSMINUS
                 : ch[1] == '=' ? PN_MINUSEQ
                                : PN_MINUS;
            break;

        case '<':
            kind = ch[1] == '<' && ch[2] == '=' ? PN_LELEEQ
               : ch[1] == '<' ? PN_LELE
               : ch[1] == '=' ? PN_LEEQ
                              : PN_LE;
            break;

        case '>':
            kind = ch[1] == '>' && ch[2] == '=' ? PN_GTGTEQ
                 : ch[1] == '>' ? PN_GTGT
                 : ch[1] == '=' ? PN_GTEQ
                                : PN_GT;
            break;

        case '|':
            kind = ch[1] == '|' ? PN_PIPEPIPE
                 : ch[1] == '=' ? PN_PIPEEQ
                                : PN_PIPE;
            break;

        default:
            assert(0);
    }

    eat(strlen(puncts[kind - PN_LSQUARE]));

    token->kind = kind;
    return true;
}


/*!
 * C99 6.4.9: Comments.
 */
static bool comment(token_t *token)
{
    assert(token);
    assert(*ch == '/' && (ch[1] == '*' || ch[1] == '/'));

    eat(2);

    if (ch[-1] == '*')
    {
        if (*ch == '/')
            eat(1);

        while (*ch && !(ch[-1] == '*' && *ch == '/'))
            eat(1);

        if (!*ch)
            return error("Unexpected EOF while parsing comment");

        eat(1);
    }
    else
        while (*ch && !is_nel(ch))
            eat(1);

    token->kind = TOK_COMMENT;
    return true;
}


/*!
 * C99 6.4.7: Header names.
 */
static bool header_name(token_t *token)
{
    assert(token);
    assert(*ch == '<' || *ch == '"');

    int expected = *ch == '<' ? '>' : '"';

    do
        eat(1);
    while (*ch && !is_nel(ch) && *ch != expected);

    if (*ch != expected)
        return error("Unexpected %s while parsing header name",
                     *ch ? "newline" : "EOF");

    eat(1);

    token->kind = TOK_HEADER_NAME;
    return true;
}


void pull_token(token_t *token)
{
    assert(token);

    bool success;
    skip_spaces();

    token->start.pos = ch;
    token->start.line = vec_len(g_lines) - 1;
    token->start.column = get_column(ch);

    switch (*ch)
    {
        // EOF.
        case '\0':
            //#TODO: add skipping of suddenly '\0'.
            token->kind = TOK_EOF;
            success = true;
            break;

        // Numbers.
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            success = numeric_const(token);
            break;

        // Wide character constant, string literal or identifier.
        case 'L':
            success = ch[1] == '\'' ? char_const(token)
                    : ch[1] == '"'  ? string_literal(token)
                                    : identifier(token);
            break;

        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': /* 'L' */ case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case '_':
            success = identifier(token);
            break;

        // Character consant.
        case '\'':
            success = char_const(token);
            break;

        // String literal or header name.
        case '"':
            if (parsing_header_name)
            {
                parsing_header_name = false;
                success = header_name(token);
            }
            else
                success = string_literal(token);

            break;

        // Numeric constant or punctuator.
        case '.':
            success = (isdigit(ch[1]) ? numeric_const : punctuator)(token);
            break;

        case '[': case ']': case '(': case ')': case '{': case '}': case '&':
        case '*': case '+': case '-': case '~': case '!': case '%': case '>':
        case '=': case '^': case '|': case '?': case ':': case ';': case ',':
            success = punctuator(token);
            break;

        // Header name or punctuator.
        case '<':
            if (parsing_header_name)
            {
                parsing_header_name = false;
                success = header_name(token);
            }
            else
                success = punctuator(token);

            break;

        // Comment or punctuator.
        case '/':
            if (ch[1] == '/' || ch[1] == '*')
                success = comment(token);
            else
                success = punctuator(token);

            break;

        case '#':
            parsing_pp_directive = true;
            success = punctuator(token);
            break;

        // Universal character name.
        case '\\':
            if (check_ucn())
            {
                success = identifier(token);
                break;
            }

            // Fallthrough.

        default:
            error("Unknown lexeme");
            eat(1);
            success = false;
            break;
    }

    if (!*ch)
        g_lines[vec_len(g_lines) - 1].length = get_column(ch);

    if (!success)
        token->kind = TOK_UNKNOWN;

    token->end.pos = ch - 1;
    token->end.line = vec_len(g_lines) - 1;
    token->end.column = get_column(ch - 1);
}


void tokenize(void)
{
    assert(g_lines && vec_len(g_lines) == 1);
    assert(!g_tokens);

    token_t token;
    g_tokens = new_vec(token_t, 4096);

    do
    {
        pull_token(&token);
        vec_push(g_tokens, token);
    }
    while (token.kind != TOK_EOF);
}
