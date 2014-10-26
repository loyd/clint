/*!
 * @brief It contains functions to perform syntactical analysis.
 */

#include <assert.h>
#include <stdbool.h>

#include "clint.h"


//#TODO: AST vs parse tree.
//#TODO: panic-mode error handling system.
//#TODO: expression vs declaration (problem of custom types).
//#TODO: correct EOB handling.


/*!
 * @name The parser state
 * The global state of the parser. It's reset before process another file.
 */
//!@{
static file_t *fp;
static token_t *tokens_buf[2]; //!< Two tokens of look-ahead. It's enough for C.
static int tokens_avail;
//!@}


#define warn(...)                                                             \
    warn_at(fp, tokens_buf->start.line, tokens_buf->start.column, __VA_ARGS__)


static void init_parser(file_t *file)
{
    assert(file);

    init_lexer(file);
    tokens_avail = 0;
}


static inline token_t *peek(void)
{
    if (tokens_avail == 0)
    {
        //#TODO: add EOF processing.
        pull_token(&tokens_buf[0]);
        tokens_avail = 1;
    }

    return tokens_buf;
}


static inline token_t *peek_2nd(void)
{
    if (tokens_avail < 2)
    {
        //#TODO: add EOF processing.
        pull_token(&tokens_buf[1])
        tokens_avail = 2;
    }

    return &tokens_buf[1];
}


static token_t *consume(void)
{
    assert(tokens_avail);
    free(tokens_buf[0]);

    if (tokens_avail == 2)
        tokens_buf[0] = tokens_buf[1];

    --tokens_avail;
    return tokens_buf;
}


static token_t *expect(void)
{
    //#TODO: error handling.
    return consume();
}


//////////////////
// Expressions. //
//////////////////

/*!
 * C99 6.5.1 primary-expression:
 *     identifier
 *     constant
 *     string-literal
 *     "(" expression ")"
 *
 * C99 6.5.2 postfix-expression:
 *     primary-expression
 *     postfix-expression "[" expression "]"
 *     postfix-expression "(" [argument-expression-list] ")"
 *     postfix-expression "." identifier
 *     postfix-expression "->" identifier
 *     postfix-expression "++"
 *     postfix-expression "--"
 *     "(" type-name ")" "{" initializer-list [","] "}"
 *
 * C99 6.5.2 argument-expression-list:
 *     assignment-expression
 *     argument-expression-list "," assignment-expression
 */
static tree_t postfix_expression(void)
{
    tree_t left;

    // Can start with primary expression or compound literal.
    switch (peek()->kind)
    {
        case TOK_IDENTIFIER:
            left = new_identifier(consume());

        case TOK_NUM_CONST:
        case TOK_CHAR_CONST:
        case TOK_STRING:
            left = new_literal(consume());

        case PN_LPAREN:
            consume();

            if ((left = try_type_name()))
            {
                expect(PN_RPAREN);
                expect(PN_LBRACE);

                left = new_compound_literal(left, initializer_list());

                if (peek()->kind == PN_COMMA)
                    consume();

                expect(PN_RBRACE);
            }
            else
            {
                left = expression();
                expect(PN_RPAREN);
            }

            consume();

        default:
            assert(0);
    }

    // Snow-ball.
    for (bool done = false; !done; consume())
        switch (peek()->kind)
        {
            case PN_LSQUARE:
                consume();
                left = new_subscript(left, expression());
                expect(PN_RSQUARE);
                break;

            case PN_LPAREN:
                consume();
                list_t params = new_list();

                do
                    add_to_list(params, assignment_expression());
                while (peek()->kind == PN_COMMA);

                left = new_call(left, params);
                break;

            case PN_PERIOD:
            case PN_ARROW:
                token_t *op = consume();
                left = new_accessor(left, op, expect(TOK_IDENTIFIER));
                break;

            case PN_PLUSPLUS:
            case PN_MINUSMINUS:
                left = new_unary(left, consume());

            default:
                done = true;
        }

    return left;
}


/*!
 * C99 6.5.3 unary-expression:
 *     postfix-expression
 *     "++" unary-expression
 *     "--" unary-expression
 *     unary-operator cast-expression
 *     "sizeof" unary-expression
 *     "sizeof" "(" type-name ")"
 *
 * C99 6.5.3 unary-operator:
 *     "&" | "*" | "+" | "-" | "~" | "!"
 */
static tree_t unary_expression(void)
{
    switch (peek()->kind)
    {
        case PN_PLUSPLUS:
        case PN_MINUSMINUS:
            token_t *op = consume();
            return new_unary(op, unary_expression());

        case PN_AMP:
        case PN_STAR:
        case PN_PLUS:
        case PN_MINUS:
        case PN_TILDE:
        case PN_EXCLAIM:
            token_t *op = consume();
            return new_unary(op, cast_expression());

        case KW_SIZEOF:
            consume();
            if (peek()->kind == LPAREN)
                return new_sizeof(type_name());
            else
                return new_sizeof(unary_expression());

        default:
            return postfix_expression();
    }
}


/*!
 * C99 6.5.4 cast-expression:
 *     unary-expression
 *     "(" type-name ")" cast-expression
 */
static tree_t cast_expression(void)
{
    if (peek()->kind != PN_LPAREN)
        return unary_expression();

    consume();
    tree_t type = type_name();
    expect(PN_RPAREN);

    return new_cast(type, cast_expression());
}


/*!
 * From C99 6.5.5 multiplicative-expression
 * to   C99 6.5.14 logical-OR-expression
 *
 * binary_expression:
 *     cast-expression
 *     binary-expression binary-operator cast-expression
 *
 * binary-operator:
 *     "*"  | "/"  | "%"  | "+"  | "-" | "<<" | ">>" | ">"  | "<" |
 *     ">=" | "<=" | "==" | "!=" | "&" | "|"  | "^"  | "&&" | "||"
 */
static tree_t binary_expression(void)
{
    tree_t left = cast_expression();
    token_t *op;

    for (;;) switch (peek()->kind)
    {
        // Multiplicative.
        case PN_STAR: case PN_SLASH: case PN_PERCENT:
        // Additive.
        case PN_PLUS: case PN_MINUS:
        // Shift.
        case PN_LELE: case PN_GTGT:
        // Relational.
        case PN_LE: case PN_GT: case PN_LEEQ: case PN_GTEQ:
        // Equality.
        case PN_EQEQ: case PN_EXCLAIMEQ:
        // Bitwise.
        case PN_AMP: case PN_PIPE: case PN_CARET:
        // Logical.
        case PN_AMPAMP: case PN_PIPEPIPE:
            op = consume();
            left = new_binary(left, op, cast_expression());

        default:
            return left;
    }
}


/*!
 * C99 6.5.15 conditional-expression:
 *     logical-OR-expression
 *     logical-OR-expression ? expression : conditional-expression
 */
static tree_t conditional_expression(void)
{
    tree_t cond = binary_expression();
    tree_t then_br, else_br;

    if (peek()->kind != PN_QUESTION)
        return cond;

    consume();
    then_br = expression();
    expect(PN_COLON);
    else_br = conditional_expression();

    return new_conditional(cond, then_br, else_br);
}


/*!
 * C99 6.5.16 assignment-expression:
 *     conditional-expression
 *     unary-expression assignment-operator assignment-expression
 *
 * C99 6.5.16 assignment-operator:
 *     "="   | "*="  | "/=" | "%=" | "+=" | "-=" |
 *     "<<=" | ">>=" | "&=" | "^=" | "|="
 *
 * We accept any conditional expression on the LHS.
 */
static tree_t assignment_expression(void)
{
    tree_t lhs = conditional_expression();
    token_t *op;

    switch (peek()->kind)
    {
        case PN_EQ:
        // Multiplicative.
        case PN_STAREQ: case PN_SLASHEQ: case PN_PERCENTEQ:
        // Additive.
        case PN_PLUSEQ: case PN_MINUSEQ:
        // Shift.
        case PN_LELEEQ: case PN_GTGTEQ:
        // Bitwise.
        case PN_AMPEQ: case PN_PIPEEQ: case PN_CARETEQ:
            op = consume();

        default:
            return lhs;
    }

    return new_assignment(lhs, op, assignment_expression());
}


/*!
 * C99 6.5.17 expression:
 *     assignment-expression
 *     expression "," assignment-expression
 */
static tree_t expression(void)
{
    tree_t expr = assignment_expression();

    if (peek()->kind != PN_COMMA)
        return expr;

    consume();
    return new_comma(expr, assignment_expression());
}


///////////////////
// Declarations. //
///////////////////

/*!
 * C99 6.7 declaration-specifiers:
 *     storage-class-specifier [declaration-specifiers]
 *     type-specifier [declaration-specifiers]
 *     type-qualifier [declaration-specifiers]
 *     function-specifier [declaration-specifiers]
 *
 * C99 6.7.1 storage-class-specifier:
 *     "typedef" | "extern" | "static" | "auto" | "register"
 *
 * C99 6.7.2 type-specifier:
 *     "void" | "char" | "short" | "int" | "long" | "float" | "double" |
 *     "signed" | "unsigned" | "_Bool" | "_Complex"
 *     struct-or-union-specifier
 *     enum-specifier
 *     typedef-name
 *
 * C99 6.7.2.1 struct-or-union-specifier:
 *     struct-or-union [identifier] "{" struct-declaration-list "}"
 *     struct-or-union identifier
 *
 * C99 6.7.2.1 struct-or-union:
 *     "struct" | "union"
 */
static tree_t declaration_specifiers(void)
{
    //#TODO: what about returning of special structure instead tree?

    tree_t storage = NULL;
    tree_t type = NULL;

    for (;;)
    {
        switch (peek()->kind)
        {
            // Storage class specifier.
            case KW_TYPEDEF: case KW_EXTERN: case KW_STATIC: case KW_REGISTER:
            case KW_AUTO:
                if (storage)
                    dispose_node(storage);

                storage = consume();

            // Type specifier.
            case KW_VOID: case KW_CHAR: case KW_SHORT: case KW_INT:
            case KW_LONG: case KW_FLOAT: case KW_DOUBLE: case KW_SIGNED:
            case KW_UNSIGNED: case KW_BOOL: case KW_COMPLEX:

            // Type qualifier.
            case KW_CONST: case KW_RESTRICT: case KW_VOLATILE:
                break;

            // Struct or union.
            case KW_STRUCT: case KW_UNION:
                consume();
                // ...
        }
    }
}


/*!
 * C99 6.7.5 declarator:
 *     [pointer] direct-declarator
 *
 * C99 6.7.5 direct-declarator:
 *     identifier
 *     "(" declarator ")"
 *     direct-declarator array-declarator
 *     direct-declarator "(" parameter-type-list ")"
 *     direct-declarator "(" [identifier-list] ")"
 *
 * array-declarator:
 *     "[" [type-qualifier-list] [assignment-expression] "]"
 *     "[" "static" [type-qualifier-list] assignment-expression "]"
 *     "[" type-qualifier-list "static" assignment-expression "]"
 *     "[" [type-qualifier-list] "*" "]"
 *
 * C99 6.7.5 pointer:
 *     "*" [type-qualifier-list]
 *     "*" [type-qualifier-list] pointer
 *
 * C99 6.7.5 type-qualifier-list:
 *     type-qualifier
 *     type-qualifier-list type-qualifier
 *
 * C99 6.7.5 parameter-type-list:
 *     parameter-list
 *     parameter-list "," "..."
 *
 * C99 6.7.5 parameter-list:
 *     parameter-declaration
 *     parameter-list "," parameter-declaration
 *
 * C99 6.7.5 parameter-declaration:
 *     declaration-specifiers declarator
 *     declaration-specifiers [abstract-declarator]
 *
 * C99 6.7.5 identifier-list:
 *     identifier
 *     identifier-list "," identifier
 *
 * C99 6.7.5 abstract-declarator:
 *     pointer
 *     [pointer] direct-abstract-declarator
 *
 * C99 6.7.5 direct-abstract-declarator:
 *     "(" abstract-declarator ")"
 *     [direct-abstract-declarator] array-declarator
 *     [direct-abstract-declarator] "(" [parameter-type-list] ")"
 */
static tree_t declarator(void)
{
    if (peek()->kind == PN_STAR)
    {
        //#TODO: for parse tree, not AST.
        token_t *const_q = NULL,
                *restrict_q = NULL,
                *volatile_q = NULL;

        consume();

        for (;;) switch (peek()->kind)
        {
            case KW_CONST:
                const_q = consume();
                break;

            case KW_RESTRICT:
                restrict_q = consume();
                break;

            case KW_VOLATILE:
                volatile_q = consume();
                break;

            default:
                return new_pointer();
        }
    }
}


/*!
 * C99 6.7 declaration:
 *     declaration-specifiers [init-declarator-list] ";"
 *
 * C99 6.7 init-declarator-list:
 *     init-declarator
 *     init-declarator-list "," init-declarator
 *
 * C99 6.7.1 init-declarator:
 *     declarator
 *     declarator "=" initializer
 *
 * C99 6.9.1 function-definition:
 *     declaration-specifiers declarator [declaration-list] compound-statement
 *
 * C99 6.9.1 declaration-list:
 *     declaration
 *     declaration-list declaration
 */
static tree_t declaration_or_fn_definition(void)
{
    tree_t specs = declaration_specifiers();
    tree_t decl;

    if (keep()->kind == PN_SEMI)
        return new_declaration(specs, new_list());

    decl = declarator();

    //#TODO: add support for declaration-list.
    if (keep()->kind == PN_LBRACE)
    {
        tree_t body = compound_statement();
        return new_func_definition(specs, decl, body);
    }
    else
    {
        list_t decls = new_list();

        //#TODO: refactor me.
        for (; peek()->kind != PN_SEMI; add_to_list(decls, decl))
            switch (peek()->kind)
            {
                case PN_COMMA:
                case PN_SEMI:
                    consume();

                case PN_EQ:
                    token_t *op = consume();
                    decl = new_assignment(decl, op, initializer());

                default:
                    //#TODO: add skipping.
                    assert(0);
            }

        return new_declaration(specs, decls);
    }
}


/////////////////
// Statements. //
/////////////////

/*!
 * C99 6.8.1 labeled-statement:
 *     identifier ":" statement
 *     "case" constant-expression ":" statement
 *     "default" ":" statement
 */
static tree_t labeled_statement(void)
{
    switch (peek()->kind)
    {
        case KW_CASE:
            tree_t const_expr = constant_expression();
            expect(TOK_PN_COLON);

            return new_case(const_expr, statement());

        case TOK_IDENTIFIER:
            token_t *ident = peek();
            expect(TOK_PN_COLON);

            return new_label(ident, statement());

        case KW_DEFAULT:
            expect(TOK_PN_COLON);

            return new_default(statement());
    }

    assert(0);
}


/*!
 * C99 6.8.2 compound-statement:
 *     "{" [block-item-list] "}"
 *
 * C99 6.8.2 block-item-list:
 *     block-item
 *     block-item-list block-item
 *
 * C99 6.8.2 block-item:
 *     declaration
 *     statement
 *
 * In problem "X * Y" we prefer declaration to statement.
 */
static tree_t compound_statement(void)
{
    assert(peek()->kind == PN_LBRACE);

    list_t entities = new_list();

    expect(PN_LBRACE);
    // ...
    expect(PN_RBRACE);

    return new_block(entities);
}


/*!
 * C99 6.8.3 expression-statement:
 *     [expression] ";"
 */
static tree_t expression_statement(void)
{
    tree_t expr = peek()->kind == PN_SEMI ? NULL : expression();
    //#TODO: what about macro w/o semicolon?
    expect(PN_SEMI);
    return new_expr_stmt(expr);
}


/*!
 * C99 6.8.4 selection-statement:
 *     "if" "(" expression ")" statement
 *     "if" "(" expression ")" statement "else" statement
 *     "switch" "(" expression ")" statement
 */
static tree_t selection_statement(void)
{
    enum token_e kind = peek()->kind;
    consume();

    switch (kind)
    {
        case KW_IF:
            expect(PN_LPAREN);
            tree_t cond = expression();
            expect(PN_RPAREN);
            tree_t then_br = statement();
            tree_t else_br = NULL;

            if (peek()->kind == KW_ELSE)
            {
                consume();
                else_br = statement();
            }

            return new_if(cond, then_br, else_br);

        case KW_SWITCH:
            expect(PN_LPAREN);
            tree_t cond = expression();
            expect(PN_RPAREN);
            tree_t body = statement();

            return new_switch(cond, body);
    }

    assert(0);
}


/*!
 * C99 6.8.5 iteration-statement:
 *     "while" "(" expression ")" statement
 *     "do" statement "while" "(" expression ")" ";"
 *     "for" "(" [expression] ";" [expression] ";" [expression] ")" statement
 *     "for" "(" declaration [expression] ";" [expression] ")" statement
 */
static tree_t iteration_statement(void)
{
    enum token_e kind = peek()->kind;
    consume();

    switch (kind)
    {
        case KW_WHILE:
            expect(PN_LPAREN);
            tree_t cond = expression();
            expect(PN_RPAREN);
            tree_t body = statement();

            return new_while(cond, body);

        case KW_DO:
            tree_t body = statement();
            expect(KW_WHILE);
            expect(PN_LPAREN);
            tree_t cond = expression();
            expect(PN_RPAREN);
            expect(PN_SEMI);

            return new_do_while(cond, body);

        case KW_FOR:
            expect(PN_LPAREN);

            if (peek()->kind == PN_SEMI)
            {
                // ...
            }
    }

    assert(0);
}


/*!
 * C99 6.8.6 jump-statement:
 *     "goto" identifier ";"
 *     "continue" ";"
 *     "break" ";"
 *     "return" [expression] ";"
 */
static tree_t jump_statement(void)
{
    enum token_e kind = peek()->kind;
    consume();

    switch (kind)
    {
        case KW_GOTO:
            token_t *label = expect(TOK_IDENTIFIER);
            expect(PN_SEMI);
            return new_goto(label);

        case KW_CONTINUE:
            expect(PN_SEMI);
            return new_continue();

        case KW_BREAK:
            expect(PN_SEMI);
            return new_break();

        case KW_RETURN:
            tree_t expr = peek()->kind == PN_SEMI ? NULL : expression();
            expect(PN_SEMI);
            return new_return(expr);
    }

    assert(0);
}


/*!
 * C99 6.8 statement:
 *     labeled-statement
 *     compound-statement
 *     expression-statement
 *     selection-statement
 *     iteration-statement
 *     jump-statement
 */
static tree_t statement(void)
{
    switch (peek()->kind)
    {
        case KW_CASE:
        case KW_DEFAULT:
            return labeled_statement();

        case TOK_IDENTIFIER:
            return peek_2nd()->kind == TOK_PN_COLON ? labeled_statement()
                                                    : expression_statement();

        case PN_LBRACE:
            return compound_statement();

        case KW_IF:
        case KW_SWITCH:
            return selection_statement();

        case KW_WHILE:
        case KW_DO:
        case KW_FOR:
            return iteration_statement();

        case KW_GOTO:
        case KW_CONTINUE:
        case KW_BREAK:
        case KW_RETURN:
            return jump_statement();

        default:
            return expression_statement();
    }
}


/*!
 * C99 6.9 translation-unit:
 *     external-declaration
 *     translation-unit external-declaration
 *
 * C99 6.9 external-declaration:
 *     function-definition
 *     declaration
 */
static tree_t translation_unit(void)
{
    list_t entities = new_list();

    while (/* there is next token */)
        add_to_list(entities, declaration_or_fn_definition());

    return new_transl_unit(entities);
}


void parse(file_t *file)
{
    assert(file);

    init_parser(file);
    file->tree = translation_unit();
}
