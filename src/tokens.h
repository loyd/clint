/*!
 * @brief There is provided a set of macros to work with tokens.
 */

#ifndef __TOKENS_H__
#define __TOKENS_H__

#define TOK_MAP(XX)                                                           \
    XX(TOK_EOF, "(eof)")                                                      \
    XX(TOK_UNKNOWN, "(unknown)")                                              \
    XX(TOK_COMMENT, "(comment)")                                              \
    XX(TOK_IDENTIFIER, "(identifier)")                                        \
    XX(TOK_NUM_CONST, "(numeric)")                                            \
    XX(TOK_CHAR_CONST, "(char)")                                              \
    XX(TOK_STRING, "(string)")                                                \
    XX(TOK_HEADER_NAME, "(header)")                                           \
    TOK_KW_MAP(XX)                                                            \
    TOK_PP_MAP(XX)                                                            \
    TOK_PN_MAP(XX)

/*! C99 6.4.1: Keywords. */
#define TOK_KW_MAP(XX)                                                        \
    XX(KW_BOOL, "_Bool")                                                      \
    XX(KW_COMPLEX, "_Complex")                                                \
    XX(KW_IMAGINARY, "_Imaginary")                                            \
    XX(KW_AUTO, "auto")                                                       \
    XX(KW_BREAK, "break")                                                     \
    XX(KW_CASE, "case")                                                       \
    XX(KW_CHAR, "char")                                                       \
    XX(KW_CONST, "const")                                                     \
    XX(KW_CONTINUE, "continue")                                               \
    XX(KW_DEFAULT, "default")                                                 \
    XX(KW_DO, "do")                                                           \
    XX(KW_DOUBLE, "double")                                                   \
    XX(KW_ELSE, "else")                                                       \
    XX(KW_ENUM, "enum")                                                       \
    XX(KW_EXTERN, "extern")                                                   \
    XX(KW_FLOAT, "float")                                                     \
    XX(KW_FOR, "for")                                                         \
    XX(KW_GOTO, "goto")                                                       \
    XX(KW_IF, "if")                                                           \
    XX(KW_INLINE, "inline")                                                   \
    XX(KW_INT, "int")                                                         \
    XX(KW_LONG, "long")                                                       \
    XX(KW_REGISTER, "register")                                               \
    XX(KW_RESTRICT, "restrict")                                               \
    XX(KW_RETURN, "return")                                                   \
    XX(KW_SHORT, "short")                                                     \
    XX(KW_SIGNED, "signed")                                                   \
    XX(KW_SIZEOF, "sizeof")                                                   \
    XX(KW_STATIC, "static")                                                   \
    XX(KW_STRUCT, "struct")                                                   \
    XX(KW_SWITCH, "switch")                                                   \
    XX(KW_TYPEDEF, "typedef")                                                 \
    XX(KW_UNION, "union")                                                     \
    XX(KW_UNSIGNED, "unsigned")                                               \
    XX(KW_VOID, "void")                                                       \
    XX(KW_VOLATILE, "volatile")                                               \
    XX(KW_WHILE, "while")

/*! C99 6.10: Preprocessor. */
#define TOK_PP_MAP(XX)                                                        \
    XX(PP_DEFINE, "define")                                                   \
    XX(PP_ELIF, "elif")                                                       \
    XX(PP_ELSE, "else")                                                       \
    XX(PP_ENDIF, "endif")                                                     \
    XX(PP_ERROR, "error")                                                     \
    XX(PP_IF, "if")                                                           \
    XX(PP_IFDEF, "ifdef")                                                     \
    XX(PP_IFNDEF, "ifndef")                                                   \
    XX(PP_INCLUDE, "include")                                                 \
    XX(PP_LINE, "line")                                                       \
    XX(PP_PRAGMA, "pragma")                                                   \
    XX(PP_UNDEF, "undef")

/*! C99 6.4.6: Punctuators. */
#define TOK_PN_MAP(XX)                                                        \
    XX(PN_LSQUARE, "[")                                                       \
    XX(PN_RSQUARE, "]")                                                       \
    XX(PN_LPAREN, "(")                                                        \
    XX(PN_RPAREN, ")")                                                        \
    XX(PN_LBRACE, "{")                                                        \
    XX(PN_RBRACE, "}")                                                        \
    XX(PN_PERIOD, ".")                                                        \
    XX(PN_ELLIPSIS, "...")                                                    \
    XX(PN_AMP, "&")                                                           \
    XX(PN_AMPAMP, "&&")                                                       \
    XX(PN_AMPEQ, "&=")                                                        \
    XX(PN_STAR, "*")                                                          \
    XX(PN_STAREQ, "*=")                                                       \
    XX(PN_PLUS, "+")                                                          \
    XX(PN_PLUSPLUS, "++")                                                     \
    XX(PN_PLUSEQ, "+=")                                                       \
    XX(PN_MINUS, "-")                                                         \
    XX(PN_ARROW, "->")                                                        \
    XX(PN_MINUSMINUS, "--")                                                   \
    XX(PN_MINUSEQ, "-=")                                                      \
    XX(PN_TILDE, "~")                                                         \
    XX(PN_EXCLAIM, "!")                                                       \
    XX(PN_EXCLAIMEQ, "!=")                                                    \
    XX(PN_SLASH, "/")                                                         \
    XX(PN_SLASHEQ, "/=")                                                      \
    XX(PN_PERCENT, "%")                                                       \
    XX(PN_PERCENTEQ, "%=")                                                    \
    XX(PN_LE, "<")                                                            \
    XX(PN_LELE, "<<")                                                         \
    XX(PN_LEEQ, "<=")                                                         \
    XX(PN_LELEEQ, "<<=")                                                      \
    XX(PN_GT, ">")                                                            \
    XX(PN_GTGT, ">>")                                                         \
    XX(PN_GTEQ, ">=")                                                         \
    XX(PN_GTGTEQ, ">>=")                                                      \
    XX(PN_CARET, "^")                                                         \
    XX(PN_CARETEQ, "^=")                                                      \
    XX(PN_PIPE, "|")                                                          \
    XX(PN_PIPEPIPE, "||")                                                     \
    XX(PN_PIPEEQ, "|=")                                                       \
    XX(PN_QUESTION, "?")                                                      \
    XX(PN_COLON, ":")                                                         \
    XX(PN_SEMI, ";")                                                          \
    XX(PN_EQ, "=")                                                            \
    XX(PN_EQEQ, "==")                                                         \
    XX(PN_COMMA, ",")                                                         \
    XX(PN_HASH, "#")                                                          \
    XX(PN_HASHHASH, "##")


enum token_e {
#define XX(kind, word) kind,
    TOK_MAP(XX)
#undef XX
};


typedef struct {
    unsigned line;
    unsigned column;
    char *pos;
} location_t;


typedef struct {
    enum token_e kind;
    location_t start, end;
} token_t;


typedef unsigned toknum_t;


#endif  // __TOKENS_H__
