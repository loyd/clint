/*!
 * @brief It declares nodes and functions to operate on the tree.
 */

#ifndef __TREE_H__
#define __TREE_H__


enum code_e {
    // Top level.
    TRANSL_UNIT,

    // Declarations.
    DECL,
    DECLARATOR,
    FUNCTION_DEF,

    // Direct types.
    ID_TYPE,
    STRUCT,
    UNION,
    ENUM,
    ENUMERATOR,

    // Indirect types.
    POINTER,
    ARRAY,
    FUNCTION,
    PARAMETER,

    // Statements.
    BLOCK,
    IF,
    SWITCH,
    WHILE,
    DO_WHILE,
    FOR,
    GOTO,
    CONTINUE,
    BREAK,
    RETURN,
    EXPR_STMT,

    // Labels.
    LABEL,
    DEFAULT,
    CASE,

    // Expressions.
    IDENTIFIER,
    ACCESSOR,
    COMMA,
    CALL,
    CONDITIONAL,
    SUBSCRIPT,
    UNARY,
    BINARY,
    ASSIGNMENT,
    LITERAL,
    COMPOUND_LITERAL,
    MEMBER_INIT
};


#define TREE_FIELDS
    //#TODO: add fields for comments and location.


typedef struct {
    enum code_e code;
    TREE_FIELDS
} *tree_t;


/*!
 * List interface.
 */
//!@{
struct list_entry_s {
    tree_t data;
    struct list_entry_s *next;
};

typedef struct {
    struct list_entry_s *first, *last;
} *list_t;

extern list_t create_list(void);
extern void add_to_list(list_t list, tree_t data);
//!@}


struct transl_unit_s {
    TREE_FIELDS
    list_t entities;
};


struct declaration_s {
    TREE_FIELDS
    token_t *storage;
    tree_t *type;
    list_t *declarators;
};


struct body_s {
    TREE_FIELDS
    list_t entities;
};


struct if_s {
    TREE_FIELDS
    tree_t cond;
    tree_t then_br;
    tree_t else_br;
};


struct switch_s {
    TREE_FIELDS
    tree_t cond;
    tree_t body;
};


struct while_s {
    TREE_FIELDS
    tree_t cond;
    tree_t body;
};


struct do_while_s {
    TREE_FIELDS
    tree_t cond;
    tree_t body;
};


struct for_s {
    TREE_FIELDS
    tree_t init;
    tree_t cond;
    tree_t next;
    tree_t body;
};


struct goto_s {
    TREE_FIELDS
    tree_t label;
};


struct break_s {
    TREE_FIELDS
};


struct continue_s {
    TREE_FIELDS
};


struct return_s {
    TREE_FIELDS
    tree_t result;
};


struct expr_stmt_s {
    TREE_FIELDS
    tree_t expr;
};


struct label_s {
    TREE_FIELDS
    tree_t name;
    tree_t stmt;
};


struct default_s {
    TREE_FIELDS
    tree_t stmt;
};


struct case_s {
    TREE_FIELDS
    tree_t expr;
    tree_t stmt;
};


//#TODO: add structs for the rest nodes.

#endif  // __TREE_H__
