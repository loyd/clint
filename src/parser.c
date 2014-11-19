/*!
 * @brief It contains functions to perform syntactical analysis.
 */

#include <assert.h>
#include <stdbool.h>

#include "clint.h"
#include "tokens.h"


//#TODO: preprocessor.
//#TODO: panic-mode error handling system.
//#TODO: correct EOB handling.


static toknum_t consumed;   //!< Number consumed tokens.


void init_parser(void)
{
    assert(!g_tokens);

    init_lexer();
    g_tokens = new_vec(token_t, 512);
    ++vec_len(g_tokens);  // 1-indexed.
    consumed = 0;
}


static enum token_e peek(unsigned lookahead)
{
    unsigned required = consumed + lookahead;

    if (consumed && g_tokens[vec_len(g_tokens)-1].kind == TOK_EOF)
        return TOK_EOF;

    while (vec_len(g_tokens) <= required)
    {
        token_t token;
        pull_token(&token);
        vec_push(g_tokens, token);
    }

    return g_tokens[required].kind;
}


static inline bool next_is(enum token_e kind)
{
    return peek(1) == kind;
}


static toknum_t consume(void)
{
    peek(1);
    return ++consumed;
}


static toknum_t accept(enum token_e kind)
{
    if (next_is(kind))
        return consume();

    return 0;
}


static toknum_t expect(enum token_e kind)
{
    //#TODO: error handling.
    if (!next_is(kind))
        assert(0);

    return consume();
}


////////////
// Nodes. //
////////////

#define T(type) type, 0, 0
#define finish(res) (tree_t)res


static tree_t finish_transl_unit(tree_t *entities)
{
    assert(entities);
    struct transl_unit_s *res = xmalloc(sizeof(*res));
    *res = (struct transl_unit_s){T(TRANSL_UNIT), entities};
    return finish(res);
}


static tree_t finish_declaration(tree_t specs, tree_t *decls)
{
    struct declaration_s *res = xmalloc(sizeof(*res));
    *res = (struct declaration_s){T(DECLARATION), specs, decls};
    return finish(res);
}


static tree_t finish_specifiers(toknum_t storage, toknum_t fnspec,
                                toknum_t *quals, tree_t dirtype)
{
    struct specifiers_s *res = xmalloc(sizeof(*res));
    *res = (struct specifiers_s){
        T(SPECIFIERS), storage, fnspec, quals, dirtype
    };

    return finish(res);
}


static tree_t finish_declarator(tree_t indtype, toknum_t name,
                                tree_t init, tree_t bitsize)
{
    struct declarator_s *res = xmalloc(sizeof(*res));
    *res = (struct declarator_s){T(DECLARATOR), indtype, name, init, bitsize};
    return finish(res);
}


static tree_t finish_function_def(tree_t specs, tree_t decl,
                                  tree_t *old_decls, tree_t body)
{
    assert(decl && body);
    struct function_def_s *res = xmalloc(sizeof(*res));
    *res = (struct function_def_s){
        T(FUNCTION_DEF), specs, decl, old_decls, body
    };

    return finish(res);
}


static tree_t finish_parameter(tree_t specs, tree_t decl)
{
    struct parameter_s *res = xmalloc(sizeof(*res));
    *res = (struct parameter_s){T(PARAMETER), specs, decl};
    return finish(res);
}


static tree_t finish_type_name(tree_t specs, tree_t decl)
{
    struct type_name_s *res = xmalloc(sizeof(*res));
    *res = (struct type_name_s){T(TYPE_NAME), specs, decl};
    return finish(res);
}


static tree_t finish_id_type(toknum_t *names)
{
    assert(names);
    struct id_type_s *res = xmalloc(sizeof(*res));
    *res = (struct id_type_s){T(ID_TYPE), names};
    return finish(res);
}


static tree_t finish_struct(toknum_t name, tree_t *members)
{
    struct struct_s *res = xmalloc(sizeof(*res));
    *res = (struct struct_s){T(STRUCT), name, members};
    return finish(res);
}


static tree_t finish_union(toknum_t name, tree_t *members)
{
    struct union_s *res = xmalloc(sizeof(*res));
    *res = (struct union_s){T(UNION), name, members};
    return finish(res);
}


static tree_t finish_enum(toknum_t name, tree_t *values)
{
    struct enum_s *res = xmalloc(sizeof(*res));
    *res = (struct enum_s){T(ENUM), name, values};
    return finish(res);
}


static tree_t finish_enumerator(toknum_t name, tree_t value)
{
    assert(name);
    struct enumerator_s *res = xmalloc(sizeof(*res));
    *res = (struct enumerator_s){T(ENUMERATOR), name, value};
    return finish(res);
}


static tree_t finish_pointer(tree_t indtype, tree_t specs)
{
    struct pointer_s *res = xmalloc(sizeof(*res));
    *res = (struct pointer_s){T(POINTER), indtype, specs};
    return finish(res);
}


static tree_t finish_array(tree_t indtype, tree_t dim_specs, tree_t dim)
{
    struct array_s *res = xmalloc(sizeof(*res));
    *res = (struct array_s){T(ARRAY), indtype, dim_specs, dim};
    return finish(res);
}


static tree_t finish_function(tree_t indtype, tree_t *params)
{
    assert(params);
    struct function_s *res = xmalloc(sizeof(*res));
    *res = (struct function_s){T(FUNCTION), indtype, params};
    return finish(res);
}


static tree_t finish_block(tree_t *entities)
{
    assert(entities);
    struct block_s *res = xmalloc(sizeof(*res));
    *res = (struct block_s){T(BLOCK), entities};
    return finish(res);
}


static tree_t finish_if(tree_t cond, tree_t then_br, tree_t else_br)
{
    assert(cond && then_br);
    struct if_s *res = xmalloc(sizeof(*res));
    *res = (struct if_s){T(IF), cond, then_br, else_br};
    return finish(res);
}


static tree_t finish_switch(tree_t cond, tree_t body)
{
    assert(cond && body);
    struct switch_s *res = xmalloc(sizeof(*res));
    *res = (struct switch_s){T(SWITCH), cond, body};
    return finish(res);
}


static tree_t finish_while(tree_t cond, tree_t body)
{
    assert(cond && body);
    struct while_s *res = xmalloc(sizeof(*res));
    *res = (struct while_s){T(WHILE), cond, body};
    return finish(res);
}


static tree_t finish_do_while(tree_t body, tree_t cond)
{
    assert(body && cond);
    struct do_while_s *res = xmalloc(sizeof(*res));
    *res = (struct do_while_s){T(DO_WHILE), cond, body};
    return finish(res);
}


static tree_t finish_for(tree_t init, tree_t cond, tree_t next, tree_t body)
{
    assert(body);
    struct for_s *res = xmalloc(sizeof(*res));
    *res = (struct for_s){T(FOR), init, cond, next, body};
    return finish(res);
}


static tree_t finish_goto(toknum_t label)
{
    assert(label);
    struct goto_s *res = xmalloc(sizeof(*res));
    *res = (struct goto_s){T(GOTO), label};
    return finish(res);
}


static tree_t finish_break(void)
{
    struct break_s *res = xmalloc(sizeof(*res));
    *res = (struct break_s){T(BREAK)};
    return finish(res);
}


static tree_t finish_continue(void)
{
    struct continue_s *res = xmalloc(sizeof(*res));
    *res = (struct continue_s){T(CONTINUE)};
    return finish(res);
}


static tree_t finish_return(tree_t result)
{
    struct return_s *res = xmalloc(sizeof(*res));
    *res = (struct return_s){T(RETURN), result};
    return finish(res);
}


static tree_t finish_label(toknum_t name, tree_t stmt)
{
    assert(name && stmt);
    struct label_s *res = xmalloc(sizeof(*res));
    *res = (struct label_s){T(LABEL), name, stmt};
    return finish(res);
}


static tree_t finish_default(tree_t stmt)
{
    assert(stmt);
    struct default_s *res = xmalloc(sizeof(*res));
    *res = (struct default_s){T(DEFAULT), stmt};
    return finish(res);
}


static tree_t finish_case(tree_t expr, tree_t stmt)
{
    assert(stmt);
    struct case_s *res = xmalloc(sizeof(*res));
    *res = (struct case_s){T(CASE), expr, stmt};
    return finish(res);
}


static tree_t finish_identifier(toknum_t value)
{
    struct identifier_s *res = xmalloc(sizeof(*res));
    *res = (struct identifier_s){T(IDENTIFIER), value};
    return finish(res);
}


static tree_t finish_constant(toknum_t value)
{
    struct constant_s *res = xmalloc(sizeof(*res));
    *res = (struct constant_s){T(CONSTANT), value};
    return finish(res);
}


static tree_t finish_special(toknum_t value)
{
    struct special_s *res = xmalloc(sizeof(*res));
    *res = (struct special_s){T(SPECIAL), value};
    return finish(res);
}


static tree_t finish_empty(void)
{
    struct empty_s *res = xmalloc(sizeof(*res));
    *res = (struct empty_s){T(EMPTY)};
    return finish(res);
}


static tree_t finish_accessor(tree_t left, toknum_t op, toknum_t field)
{
    assert(left && op && field);
    struct accessor_s *res = xmalloc(sizeof(*res));
    *res = (struct accessor_s){T(ACCESSOR), left, op, field};
    return finish(res);
}


static tree_t finish_comma(tree_t *exprs)
{
    assert(exprs);
    struct comma_s *res = xmalloc(sizeof(*res));
    *res = (struct comma_s){T(COMMA), exprs};
    return finish(res);
}


static tree_t finish_call(tree_t left, tree_t *args)
{
    assert(left && args);
    struct call_s *res = xmalloc(sizeof(*res));
    *res = (struct call_s){T(CALL), left, args};
    return finish(res);
}

static tree_t finish_cast(tree_t type_name, tree_t expr)
{
    assert(type_name && expr);
    struct cast_s *res = xmalloc(sizeof(*res));
    *res = (struct cast_s){T(CAST), type_name, expr};
    return finish(res);
}



static tree_t finish_conditional(tree_t cond, tree_t then_br, tree_t else_br)
{
    assert(cond && then_br && else_br);
    struct conditional_s *res = xmalloc(sizeof(*res));
    *res = (struct conditional_s){T(CONDITIONAL), cond, then_br, else_br};
    return finish(res);
}


static tree_t finish_subscript(tree_t left, tree_t index)
{
    assert(left && index);
    struct subscript_s *res = xmalloc(sizeof(*res));
    *res = (struct subscript_s){T(SUBSCRIPT), left, index};
    return finish(res);
}


static tree_t finish_unary(toknum_t op, tree_t expr)
{
    assert(op && expr);
    struct unary_s *res = xmalloc(sizeof(*res));
    *res = (struct unary_s){T(UNARY), op, expr};
    return finish(res);
}


static tree_t finish_binary(tree_t left, toknum_t op, tree_t right)
{
    assert(left && op && right);
    struct binary_s *res = xmalloc(sizeof(*res));
    *res = (struct binary_s){T(BINARY), left, op, right};
    return finish(res);
}


static tree_t finish_assignment(tree_t left, toknum_t op, tree_t right)
{
    assert(left && op && right);
    struct assignment_s *res = xmalloc(sizeof(*res));
    *res = (struct assignment_s){T(ASSIGNMENT), left, op, right};
    return finish(res);
}


static tree_t finish_comp_literal(tree_t type_name, tree_t *members)
{
    assert(members);
    struct comp_literal_s *res = xmalloc(sizeof(*res));
    *res = (struct comp_literal_s){T(COMP_LITERAL), type_name, members};
    return finish(res);
}


static tree_t finish_comp_member(tree_t *designs, tree_t init)
{
    assert(init);
    struct comp_member_s *res = xmalloc(sizeof(*res));
    *res = (struct comp_member_s){T(COMP_MEMBER), designs, init};
    return finish(res);
}


/*!
 * In problem "X(Y)" we prefer expression to declaration.
 * In problem "X Y" we prefer declaration to expression (w/ macros).
 *
 * In normal mode:
 *     "X * Y"      expression
 *     "X)" "X,"    expression
 *
 * In agressive mode:
 *     "X * Y"      declaration
 *     "X)" "X,"    declaration
 */
static bool starts_declaration(bool agressive)
{
    switch (peek(1))
    {
        // Custom type or start of expression.
        case TOK_IDENTIFIER:
            switch (peek(2))
            {
                case PN_RPAREN:
                case PN_COMMA:
                    return agressive;

                case PN_STAR:
                    // "X *)" is are always declaration.
                    if (peek(3) == PN_RPAREN)
                        return true;

                    return agressive;// && peek(3) == TOK_IDENTIFIER;

                case TOK_IDENTIFIER: case KW_TYPEDEF:
                case KW_EXTERN: case KW_STATIC: case KW_REGISTER: case KW_AUTO:
                case KW_CONST: case KW_RESTRICT: case KW_VOLATILE:
                    return true;
            }

            break;

        // Storage class specifiers.
        case KW_TYPEDEF: case KW_EXTERN: case KW_STATIC: case KW_REGISTER:
        case KW_AUTO:
        // Primitive type specifiers.
        case KW_VOID: case KW_CHAR: case KW_SHORT: case KW_INT:
        case KW_LONG: case KW_FLOAT: case KW_DOUBLE: case KW_SIGNED:
        case KW_UNSIGNED: case KW_BOOL: case KW_COMPLEX:
        // Type qualifiers.
        case KW_CONST: case KW_RESTRICT: case KW_VOLATILE:
        // Structures.
        case KW_STRUCT: case KW_UNION: case KW_ENUM:
        // Function specifier.
        case KW_INLINE:
            return true;
    }

    return false;
}


static tree_t cast_expression(bool after_sizeof);
static tree_t postfix_expression_suffixes(tree_t left);
static tree_t binary_expression(void);
static tree_t conditional_expression(void);
static tree_t assignment_expression(void);
static tree_t expression(void);
static tree_t constant_expression(void);

static tree_t declaration(void);
static tree_t declaration_inner(tree_t specs, tree_t first_declarator);
static tree_t declaration_specifiers(bool agressive);
static tree_t struct_or_union_specifier(void);
static tree_t enum_specifier(void);
static tree_t init_declarator(void);
static tree_t declarator_inner(toknum_t *name);
static tree_t direct_declarator_inner(toknum_t *name);
static tree_t compound_literal(tree_t type);
static tree_t initializer(void);
static tree_t type_name(void);
static tree_t declaration_or_fn_definition(void);

static tree_t statement(void);
static tree_t labeled_statement(void);
static tree_t compound_statement(void);
static tree_t expression_statement(void);
static tree_t selection_statement(void);
static tree_t iteration_statement(void);
static tree_t jump_statement(void);

static tree_t translation_unit(void);


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
 *     postfix-expression postfix-expression-suffix
 *     "(" type-name ")" "{" initializer-list [","] "}"
 *
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
 *
 * C99 6.5.4 cast-expression:
 *     unary-expression
 *     "(" type-name ")" cast-expression
 */
static tree_t cast_expression(bool after_sizeof)
{
    tree_t left;

    switch (peek(1))
    {
        // Compound literal: (<type-name>) {<init-list>}
        // Cast expression:  (<type-name>) <cast-expr>          [!after_sizeof]
        // After sizeof: (<type-name>)                          [after_sizeof]
        // Postfix expression: (<expr>) <postfix-expr-suffix>
        // Primary expression: (<expr>)
        case PN_LPAREN:
            consume();

            // Choice between type name and expression.
            if (starts_declaration(false))
                left = type_name();
            else if (next_is(TOK_IDENTIFIER) && peek(2) == PN_RPAREN)
            {
                // Some heuristics.
                switch (peek(3))
                {
                    case PN_SEMI:
                    case PN_COMMA:
                    case PN_RPAREN:
                        left = after_sizeof ? type_name() : expression();
                        break;

                    case PN_ARROW:
                    case PN_PERIOD:
                    case PN_LSQUARE:
                        left = expression();
                        break;

                    case PN_PLUSPLUS:
                    case PN_MINUSMINUS:
                        left = peek(4) == TOK_IDENTIFIER ? type_name()
                                                         : expression();
                        break;

                    default:
                        left = type_name();
                }
            }
            else
                left = expression();

            expect(PN_RPAREN);

            if (left->type == TYPE_NAME)
                if (next_is(PN_LBRACE))
                    left = compound_literal(left);
                else if (after_sizeof)
                    return left;
                else
                    return finish_cast(left, cast_expression(false));

            break;

        // Primary expression.
        case TOK_IDENTIFIER:
            left = finish_identifier(consume());
            break;

        case TOK_NUM_CONST:
        case TOK_CHAR_CONST:
        case TOK_STRING:
            left = finish_constant(consume());
            break;

        // Prefix unary operators.
        case PN_PLUSPLUS:
        case PN_MINUSMINUS:
        case PN_AMP:
        case PN_STAR:
        case PN_PLUS:
        case PN_MINUS:
        case PN_TILDE:
        case PN_EXCLAIM:
        {
            toknum_t op = consume();
            return finish_unary(op, cast_expression(false));
        }

        // `sizeof` operator.
        case KW_SIZEOF:
        {
            toknum_t op = consume();
            return finish_unary(op, cast_expression(true));
        }
    }

    return postfix_expression_suffixes(left);
}


/*!
 * postfix-expression-suffixes:
 *     [postfix-expression-suffix]*
 *
 * postfix-expression-suffix:
 *     "[" expression "]"
 *     "(" [argument-expression-list] ")"
 *     "." identifier
 *     "->" identifier
 *     "++"
 *     "--"
 *
 * C99 6.5.2 argument-expression-list:
 *     [argument-expression-list ","] assignment-expression
 */
static tree_t postfix_expression_suffixes(tree_t left)
{
    for(;;) switch (peek(1))
    {
        case PN_LSQUARE:
            consume();
            left = finish_subscript(left, expression());
            expect(PN_RSQUARE);
            break;

        case PN_LPAREN:
        {
            tree_t *args = new_vec(tree_t, 2);
            consume();

            while (!accept(PN_RPAREN))
            {
                vec_push(args, assignment_expression());
                if (!next_is(PN_RPAREN))
                    expect(PN_COMMA);
            }

            left = finish_call(left, args);
            break;
        }

        case PN_PERIOD:
        case PN_ARROW:
        {
            toknum_t op = consume();
            left = finish_accessor(left, op, expect(TOK_IDENTIFIER));
            break;
        }

        case PN_PLUSPLUS:
        case PN_MINUSMINUS:
            left = finish_unary(consume(), left);
            break;

        default:
            return left;
    }
}


/*!
 * From C99 6.5.5 multiplicative-expression
 * to   C99 6.5.14 logical-OR-expression
 *
 * binary-expression:
 *     [binary-expression binary-operator] cast-expression
 *
 * binary-operator:
 *     "*"  | "/"  | "%"  | "+"  | "-" | "<<" | ">>" | ">"  | "<" |
 *     ">=" | "<=" | "==" | "!=" | "&" | "|"  | "^"  | "&&" | "||"
 */
static tree_t binary_expression(void)
{
    tree_t left = cast_expression(false);
    toknum_t op;

    for (;;) switch (peek(1))
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
            left = finish_binary(left, op, cast_expression(false));
            break;

        default:
            return left;
    }
}


/*!
 * C99 6.5.15 conditional-expression:
 *     logical-OR-expression ["?" expression ":" conditional-expression]
 */
static tree_t conditional_expression(void)
{
    tree_t cond = binary_expression();
    tree_t then_br, else_br;

    if (!accept(PN_QUESTION))
        return cond;

    then_br = expression();
    expect(PN_COLON);
    else_br = conditional_expression();

    return finish_conditional(cond, then_br, else_br);
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
    toknum_t op;

    switch (peek(1))
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
            break;

        default:
            return lhs;
    }

    return finish_assignment(lhs, op, assignment_expression());
}


/*!
 * C99 6.5.17 expression:
 *     [expression ","] assignment-expression
 */
static tree_t expression(void)
{
    tree_t expr = assignment_expression();
    tree_t *exprs;

    if (!next_is(PN_COMMA))
        return expr;

    exprs = new_vec(tree_t, 2);
    vec_push(exprs, expr);

    while (accept(PN_COMMA))
        vec_push(exprs, assignment_expression());

    return finish_comma(exprs);
}


/*!
 * C99 6.6 constant-expression:
 *     conditional-expression
 */
static inline tree_t constant_expression(void)
{
    return conditional_expression();
}


///////////////////
// Declarations. //
///////////////////

/*!
 * C99 6.7 declaration:
 *     declaration-specifiers [init-declarator-list] ";"
 *
 * C99 6.7 init-declarator-list:
 *     init-declarator
 *     init-declarator-list "," init-declarator
 */
static tree_t declaration(void)
{
    tree_t specs = declaration_specifiers(false);

    if (accept(PN_SEMI))
        return finish_declaration(specs, NULL);

    return declaration_inner(specs, init_declarator());
}


static tree_t declaration_inner(tree_t specs, tree_t first_declarator)
{
    tree_t *decls = new_vec(tree_t, 1);
    vec_push(decls, first_declarator);

    while (accept(PN_COMMA))
        vec_push(decls, init_declarator());

    expect(PN_SEMI);
    return finish_declaration(specs, decls);
}


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
 * C99 6.7.3 type-qualifier:
 *     "const" | "restrict" | "volatile"
 *
 * C99 6.7.4 function-specifier:
 *     "inline"
 *
 * We accept empty declaration specifiers.
 */
static tree_t declaration_specifiers(bool agressive)
{
    toknum_t storage = 0;
    toknum_t fnspec = 0;
    tree_t dirtype = NULL;
    toknum_t *names = NULL;
    toknum_t *quals = NULL;

    for (;;) switch (peek(1))
    {
        // Storage class specifiers.
        case KW_TYPEDEF: case KW_EXTERN: case KW_STATIC: case KW_REGISTER:
        case KW_AUTO:
            storage = consume();
            break;

        // Primitive type specifiers.
        case KW_VOID: case KW_CHAR: case KW_SHORT: case KW_INT:
        case KW_LONG: case KW_FLOAT: case KW_DOUBLE: case KW_SIGNED:
        case KW_UNSIGNED: case KW_BOOL: case KW_COMPLEX:
            if (!names)
                names = new_vec(toknum_t, 1);

            vec_push(names, consume());
            break;

        // Type qualifiers.
        case KW_CONST: case KW_RESTRICT: case KW_VOLATILE:
            if (!quals)
                quals = new_vec(toknum_t, 1);

            vec_push(quals, consume());
            break;

        // Struct or union.
        case KW_STRUCT: case KW_UNION:
            if (dirtype)
                dispose_tree(dirtype);

            dirtype = struct_or_union_specifier();
            break;

        // Enumeration.
        case KW_ENUM:
            if (dirtype)
                dispose_tree(dirtype);

            dirtype = enum_specifier();
            break;

        // Function specifier.
        case KW_INLINE:
            fnspec = consume();
            break;

        // Perhaps custom type.
        case TOK_IDENTIFIER:
            if (!(dirtype || names) && starts_declaration(agressive))
            {
                names = new_vec(toknum_t, 1);
                vec_push(names, consume());
            }

            // Fallthrough.

        default:
            if (names)
            {
                if (dirtype)
                    dispose_tree(dirtype);

                dirtype = finish_id_type(names);
            }

            if (!(dirtype || storage || quals || fnspec))
                return NULL;

            return finish_specifiers(storage, fnspec, quals, dirtype);
    }
}


/*!
 * C99 6.7.2.1 struct-or-union-specifier:
 *     struct-or-union [identifier] "{" struct-declaration-list "}"
 *     struct-or-union identifier
 *
 * C99 6.7.2.1 struct-or-union:
 *     "struct" | "union"
 *
 * C99 6.7.2.1 struct-declaration-list:
 *     [struct-declaration]+
 *
 * C99 6.7.2.1 struct-declaration:
 *     specifier-qualifier-list struct-declarator-list ";"
 *
 * C99 6.7.2.1 specifier-qualifier-list:
 *     type-specifier [specifier-qualifier-list]
 *     type-qualifier [specifier-qualifier-list]
 *
 * C99 6.7.2.1 struct-declarator-list:
 *     [struct-declarator-list ","] struct-declarator
 *
 * C99 6.7.2.1 struct-declarator:
 *     declarator
 *     [declarator] ":" constant-expression
 */
static tree_t struct_or_union_specifier(void)
{
    assert(next_is(KW_STRUCT) || next_is(KW_UNION));

    tree_t (*ctor)(toknum_t, tree_t *);
    toknum_t name;
    tree_t *members;

    ctor = peek(1) == KW_UNION ? finish_union : finish_struct;
    consume();

    // Parse name.
    name = accept(TOK_IDENTIFIER);

    if (!accept(PN_LBRACE))
        return ctor(name, NULL);

    members = new_vec(tree_t, 4);

    // Members.
    while (!accept(PN_RBRACE))
        vec_push(members, declaration());

    return ctor(name, members);
}


/*!
 * C99 6.7.2.2 enum-specifier:
 *     "enum" [identifier] "{" enumerator-list [","] "}"
 *     "enum" identifier
 *
 * C99 6.7.2.2 enumerator-list:
 *     enumerator
 *     enumerator-list "," enumerator
 *
 * C99 6.7.2.2 enumerator:
 *     enumeration-constant ["=" constant-expression]
 */
static tree_t enum_specifier(void)
{
    tree_t *enumerators;
    toknum_t enum_name = 0;

    expect(KW_ENUM);

    if (next_is(TOK_IDENTIFIER))
    {
        enum_name = consume();
        if (!next_is(PN_LBRACE))
            return finish_enum(enum_name, NULL);
    }

    expect(PN_LBRACE);
    enumerators = new_vec(tree_t, 4);

    while (!accept(PN_RBRACE))
    {
        toknum_t enumerator_name = expect(TOK_IDENTIFIER);
        tree_t enumerator = finish_enumerator(enumerator_name,
            accept(PN_EQ) ? constant_expression() : NULL);

        vec_push(enumerators, enumerator);
        accept(PN_COMMA);
    }

    return finish_enum(enum_name, enumerators);
}


/*!
 * C99 6.7.1 init-declarator:
 *     declarator ["=" initializer]
 *
 * C99 6.7.2.1 struct-declarator:
 *     declarator
 *     [declarator] ":" constant-expression
 */
static tree_t init_declarator(void)
{
    toknum_t name = 0;
    tree_t init = NULL;
    tree_t indtype = NULL;
    tree_t bitsize = NULL;

    if (!next_is(PN_COLON))
        indtype = declarator_inner(&name);

    if (accept(PN_EQ))
        init = initializer();
    else if (accept(PN_COLON))
        bitsize = constant_expression();

    return finish_declarator(indtype, name, init, bitsize);
}


/*!
 * C99 6.7.5 declarator:
 *     [pointer] direct-declarator
 *
 * C99 6.7.5 abstract-declarator:
 *     pointer
 *     [pointer] direct-abstract-declarator
 *
 * C99 6.7.5 pointer:
 *     ["*" [type-qualifier-list]]+
 *
 * C99 6.7.5 type-qualifier-list:
 *     [type-qualifier]+
 */
static tree_t declarator_inner(toknum_t *name)
{
    tree_t specs;

    if (!accept(PN_STAR))
        return direct_declarator_inner(name);

    specs = declaration_specifiers(false);

    switch (peek(1))
    {
        // Nested pointer.
        case PN_STAR:
        // Signs of (abstract) direct declarator.
        case TOK_IDENTIFIER:
        case PN_LSQUARE:
        case PN_LPAREN:
            return finish_pointer(declarator_inner(name), specs);
    }

    // Unexpected abstract declarator.
    if (name)
        *name = 0;

    // Only pointer within abstract declarator.
    return finish_pointer(NULL, specs);
}


/*!
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
 * C99 6.7.5 parameter-type-list:
 *     parameter-list ["," "..."]
 *
 * C99 6.7.5 parameter-list:
 *     [parameter-list ","] parameter-declaration
 *
 * C99 6.7.5 parameter-declaration:
 *     declaration-specifiers declarator
 *     declaration-specifiers [abstract-declarator]
 *
 * C99 6.7.5 identifier-list:
 *     [identifier-list ","] identifier
 *
 * C99 6.7.5 direct-abstract-declarator:
 *     "(" abstract-declarator ")"
 *     [direct-abstract-declarator] array-declarator
 *     [direct-abstract-declarator] "(" [parameter-type-list] ")"
 */
static tree_t direct_declarator_inner(toknum_t *name)
{
    tree_t indtype = NULL;
    toknum_t ident = 0;

    for (;;) switch (peek(1))
    {
        case TOK_IDENTIFIER:
            ident = consume();
            break;

        // Array declarator.
        case PN_LSQUARE:
        {
            tree_t dim_specs;
            tree_t dimension = NULL;

            consume();
            dim_specs = declaration_specifiers(false);

            if (next_is(PN_STAR))
                dimension = finish_special(consume());
            else if (!next_is(PN_RSQUARE))
                dimension = assignment_expression();

            indtype = finish_array(indtype, dim_specs, dimension);
            expect(PN_RSQUARE);
            break;
        }

        // Function or group.
        case PN_LPAREN:
        {
            bool is_group;
            consume();

            if (name)
                is_group = !ident;
            else if (next_is(PN_RPAREN) || starts_declaration(true))
                is_group = false;
            else
                is_group = true;

            if (is_group)
            {
                indtype = declarator_inner(&ident);
                expect(PN_RPAREN);
                break;
            }

            tree_t *params = new_vec(tree_t, 2);

            while (!accept(PN_RPAREN))
            {
                if (next_is(PN_ELLIPSIS))
                    vec_push(params, finish_special(consume()));
                else
                {
                    tree_t specs = declaration_specifiers(true);
                    tree_t declarator = NULL;

                    if (!(next_is(PN_COMMA) || next_is(PN_RPAREN)))
                        declarator = init_declarator();

                    vec_push(params, finish_parameter(specs, declarator));
                }

                if (!next_is(PN_RPAREN))
                    expect(PN_COMMA);
            }

            indtype = finish_function(indtype, params);
            break;
        }

        default:
            if (name)
                *name = ident;

            return indtype;
    }
}


/*!
 * compound-literal:
 *     "{" initializer-list [","] "}"
 *
 * C99 6.7.8 initializer-list:
 *     [initializer-list ","] [designation] initializer
 *
 * C99 6.7.8 designation:
 *     [designator]+ "="
 *
 * C99 6.7.8 designator:
 *     "[" constant-expression "]"
 *     "." identifier
 */
static tree_t compound_literal(tree_t type)
{
    tree_t *members = new_vec(tree_t, 3);
    expect(PN_LBRACE);

    while (!accept(PN_RBRACE))
    {
        tree_t *designators = NULL;

        // Parse any designators.
        while (next_is(PN_LSQUARE) || next_is(PN_PERIOD))
        {
            if (!designators)
                designators = new_vec(tree_t, 1);

            if (accept(PN_LSQUARE))
            {
                vec_push(designators, constant_expression());
                expect(PN_RSQUARE);
            }
            else if (accept(PN_PERIOD))
                vec_push(designators, finish_identifier(consume()));
        }

        if (designators)
            expect(PN_EQ);

        vec_push(members, finish_comp_member(designators, initializer()));

        if (!next_is(PN_RBRACE))
            expect(PN_COMMA);
    }

    return finish_comp_literal(type, members);
}


/*!
 * C99 6.7.8 initializer:
 *     assignment-expression
 *     compound-literal
 */
static tree_t initializer(void)
{
    return next_is(PN_LBRACE) ? compound_literal(NULL)
                              : assignment_expression();
}


/*!
 * C99 6.7.6 type-name:
 *     specifier-qualifier-list [abstract-declarator]
 */
static tree_t type_name(void)
{
    tree_t specs = declaration_specifiers(true);
    tree_t decl = declarator_inner(NULL);

    return finish_type_name(specs, decl);
}


/*!
 * C99 6.7 declaration:
 *     declaration-specifiers [init-declarator-list] ";"
 *
 * C99 6.9.1 function-definition:
 *     declaration-specifiers declarator [declaration-list] compound-statement
 *
 * C99 6.9.1 declaration-list:
 *     [declaration]+
 */
static tree_t declaration_or_fn_definition(void)
{
    tree_t specs = declaration_specifiers(true);
    struct declarator_s *declarator;
    bool is_function = false;

    if (accept(PN_SEMI))
        return finish_declaration(specs, NULL);

    declarator = (struct declarator_s *)init_declarator();

    // Check last indirect type.
    for (tree_t i = (tree_t)declarator; i; i = ((struct pointer_s *)i)->indtype)
        is_function = i->type == FUNCTION;

    // Function definition.
    if (is_function && !declarator->init && !next_is(PN_SEMI))
    {
        tree_t *old_decls = NULL;
        tree_t body;

        // Old-style declaration list.
        if (!next_is(PN_LBRACE))
        {
            old_decls = new_vec(tree_t, 2);
            while (!next_is(PN_LBRACE))
                vec_push(old_decls, declaration());
        }

        body = compound_statement();
        return finish_function_def(specs, (tree_t)declarator, old_decls, body);
    }

    // Declaration, otherwise.
    return declaration_inner(specs, (tree_t)declarator);
}


/////////////////
// Statements. //
/////////////////

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
    switch (peek(1))
    {
        case KW_CASE:
        case KW_DEFAULT:
            return labeled_statement();

        case TOK_IDENTIFIER:
            return peek(2) == PN_COLON ? labeled_statement()
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
    }

    return expression_statement();
}


/*!
 * C99 6.8.1 labeled-statement:
 *     identifier ":" statement
 *     "case" constant-expression ":" statement
 *     "default" ":" statement
 */
static tree_t labeled_statement(void)
{
    switch (peek(1))
    {
        case KW_CASE:
        {
            tree_t const_expr;
            consume();
            const_expr = constant_expression();
            expect(PN_COLON);

            return finish_case(const_expr, statement());
        }

        case TOK_IDENTIFIER:
        {
            toknum_t ident = consume();
            expect(PN_COLON);

            return finish_label(ident, statement());
        }

        case KW_DEFAULT:
            consume();
            expect(PN_COLON);

            return finish_default(statement());
    }

    return NULL;
}


/*!
 * C99 6.8.2 compound-statement:
 *     "{" [block-item-list] "}"
 *
 * C99 6.8.2 block-item-list:
 *     [block-item]+
 *
 * C99 6.8.2 block-item:
 *     declaration
 *     statement
 */
static tree_t compound_statement(void)
{
    tree_t *entities = new_vec(tree_t, 8);
    expect(PN_LBRACE);

    while (!accept(PN_RBRACE))
        vec_push(entities, starts_declaration(true) ? declaration()
                                                       : statement());

    return finish_block(entities);
}


/*!
 * C99 6.8.3 expression-statement:
 *     [expression] ";"
 */
static tree_t expression_statement(void)
{
    tree_t expr = next_is(PN_SEMI) ? finish_empty() : expression();
    accept(PN_SEMI);
    return expr;
}


/*!
 * C99 6.8.4 selection-statement:
 *     "if" "(" expression ")" statement ["else" statement]
 *     "switch" "(" expression ")" statement
 */
static tree_t selection_statement(void)
{
    if (accept(KW_IF))
    {
        tree_t cond, then_br, else_br = NULL;

        expect(PN_LPAREN);
        cond = expression();
        expect(PN_RPAREN);
        then_br = statement();

        if (accept(KW_ELSE))
            else_br = statement();

        return finish_if(cond, then_br, else_br);
    }

    if (accept(KW_SWITCH))
    {
        tree_t cond, body;

        expect(PN_LPAREN);
        cond = expression();
        expect(PN_RPAREN);
        body = statement();

        return finish_switch(cond, body);
    }

    return NULL;
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
    tree_t cond, body;
    enum token_e kind = peek(1);
    consume();

    switch (kind)
    {
        case KW_WHILE:
            expect(PN_LPAREN);
            cond = expression();
            expect(PN_RPAREN);
            body = statement();

            return finish_while(cond, body);

        case KW_DO:
            body = statement();
            expect(KW_WHILE);
            expect(PN_LPAREN);
            cond = expression();
            expect(PN_RPAREN);
            expect(PN_SEMI);

            return finish_do_while(body, cond);

        case KW_FOR:
        {
            tree_t init = NULL, next;
            expect(PN_LPAREN);

            if (next_is(PN_SEMI))
                consume();
            else if (starts_declaration(true))
                init = declaration();
            else
            {
                init = expression();
                expect(PN_SEMI);
            }

            cond = next_is(PN_SEMI) ? NULL : expression();
            expect(PN_SEMI);
            next = next_is(PN_RPAREN) ? NULL : expression();
            expect(PN_RPAREN);
            body = statement();

            return finish_for(init, cond, next, body);
        }

        default:
            assert(0);
    }
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
    tree_t stmt = NULL;
    enum token_e kind = peek(1);
    consume();

    switch (kind)
    {
        case KW_GOTO:
            stmt = finish_goto(expect(TOK_IDENTIFIER));
            break;

        case KW_CONTINUE:
            stmt = finish_continue();
            break;

        case KW_BREAK:
            stmt = finish_break();
            break;

        case KW_RETURN:
        {
            tree_t expr = next_is(PN_SEMI) ? NULL : expression();
            stmt = finish_return(expr);
            break;
        }

        default:
            assert(0);
    }

    expect(PN_SEMI);
    return stmt;
}


/*!
 * C99 6.9 translation-unit:
 *     [external-declaration]+
 *
 * C99 6.9 external-declaration:
 *     function-definition
 *     declaration
 */
static tree_t translation_unit(void)
{
    tree_t *entities = new_vec(tree_t, 20);

    while (!accept(TOK_EOF))
        vec_push(entities, declaration_or_fn_definition());

    return finish_transl_unit(entities);
}


void parse(void)
{
    assert(g_tokens && vec_len(g_tokens) == 1);
    assert(!g_tree);
    g_tree = translation_unit();
}
