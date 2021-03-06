/*!
 * @brief It declares nodes and functions to operate on the tree.
 */

#ifndef __TREE_H__
#define __TREE_H__

#include "tokens.h"


enum type_e {
    // Top level.
    TRANSL_UNIT,        // {entities[]}
    EMPTY,              // {}

    // Declarations.
    DECLARATION,        // {specs*, decls*[]}
    SPECIFIERS,         // {#storage*, #fnspec*, #quals*[], dirtype*, attrs*[]}
    DECLARATOR,         // {indtype*, #name*, init*, bitsize*, attrs*[]}
    FUNCTION_DEF,       // {specs, decl, old_decls*[], body}
    PARAMETER,          // {specs*, decl*} (decl can be ellipsis)
    TYPE_NAME,          // {specs*, decl*} (abstract decl)

    ATTRIBUTE,          // {attribs*[]}
    ATTRIB,             // {name, args*[]}

    // Direct types.
    ID_TYPE,            // {#names[]}
    STRUCT,             // {#name*, members*[]}
    UNION,              // {#name*, members*[]}
    ENUM,               // {#name*, values*[]}
    ENUMERATOR,         // {#name, value*}

    // Indirect types.
    POINTER,            // {indtype*, specs*}
    ARRAY,              // {indtype*, dim_specs*, dim*}
    FUNCTION,           // {indtype*, params[]}

    // Statements.
    BLOCK,              // {entities[]}
    IF,                 // {cond, then_br, else_br*}
    SWITCH,             // {cond, body}
    WHILE,              // {cond, body}
    DO_WHILE,           // {cond, body}
    FOR,                // {init*, cond*, next*, body}
    GOTO,               // {#label}
    BREAK,              // {}
    CONTINUE,           // {}
    RETURN,             // {result}

    // Labels.
    LABEL,              // {#name, stmt}
    DEFAULT,            // {stmt}
    CASE,               // {expr, stmt}

    // Expressions.
    CONSTANT,           // {#value}
    IDENTIFIER,         // {#value}
    SPECIAL,            // {#value}
    ACCESSOR,           // {left, #op, #field}
    COMMA,              // {exprs[]}
    CALL,               // {left, args[]}
    CAST,               // {type_name, expr}
    CONDITIONAL,        // {cond, then_br, else_br}
    SUBSCRIPT,          // {left, index}
    UNARY,              // {#op, expr}
    BINARY,             // {left, #op, right}
    ASSIGNMENT,         // {left, #op, right}
    COMP_LITERAL,       // {type_name*, members[]}
    COMP_MEMBER         // {designs*[], init}
};


#define TREE_FIELDS                                                           \
    enum type_e type;                                                         \
    struct tree_s *parent;                                                    \
    toknum_t start, end


typedef struct tree_s {
    TREE_FIELDS;
} *tree_t;


//#TODO: add documentation.

struct transl_unit_s {
    TREE_FIELDS;
    tree_t *entities;
};


struct empty_s {
    TREE_FIELDS;
};


struct declaration_s {
    TREE_FIELDS;
    tree_t specs;
    tree_t *decls;
};


struct specifiers_s {
    TREE_FIELDS;
    toknum_t storage;
    toknum_t fnspec;
    toknum_t *quals;
    tree_t dirtype;
    tree_t *attrs;
};


struct declarator_s {
    TREE_FIELDS;
    tree_t indtype;
    toknum_t name;
    tree_t init;
    tree_t bitsize;
    tree_t *attrs;
};


struct function_def_s {
    TREE_FIELDS;
    tree_t specs;
    tree_t decl;
    tree_t *old_decls;
    tree_t body;
};


struct parameter_s {
    TREE_FIELDS;
    tree_t specs;
    tree_t decl;
};


struct type_name_s {
    TREE_FIELDS;
    tree_t specs;
    tree_t decl;
};


struct attribute_s {
    TREE_FIELDS;
    tree_t *attribs;
};


struct attrib_s {
    TREE_FIELDS;
    toknum_t name;
    tree_t *args;
};


struct id_type_s {
    TREE_FIELDS;
    toknum_t *names;
};


struct struct_s {
    TREE_FIELDS;
    toknum_t name;
    tree_t *members;
};


struct union_s {
    TREE_FIELDS;
    toknum_t name;
    tree_t *members;
};


struct enum_s {
    TREE_FIELDS;
    toknum_t name;
    tree_t *values;
};


struct enumerator_s {
    TREE_FIELDS;
    toknum_t name;
    tree_t value;
};


struct pointer_s {
    TREE_FIELDS;
    tree_t indtype;
    tree_t specs;
};


struct array_s {
    TREE_FIELDS;
    tree_t indtype;
    tree_t dim_specs;
    tree_t dim;
};


struct function_s {
    TREE_FIELDS;
    tree_t indtype;
    tree_t *params;
};


struct block_s {
    TREE_FIELDS;
    tree_t *entities;
};


struct if_s {
    TREE_FIELDS;
    tree_t cond;
    tree_t then_br;
    tree_t else_br;
};


struct switch_s {
    TREE_FIELDS;
    tree_t cond;
    tree_t body;
};


struct while_s {
    TREE_FIELDS;
    tree_t cond;
    tree_t body;
};


struct do_while_s {
    TREE_FIELDS;
    tree_t cond;
    tree_t body;
};


struct for_s {
    TREE_FIELDS;
    tree_t init;
    tree_t cond;
    tree_t next;
    tree_t body;
};


struct goto_s {
    TREE_FIELDS;
    toknum_t label;
};


struct break_s {
    TREE_FIELDS;
};


struct continue_s {
    TREE_FIELDS;
};


struct return_s {
    TREE_FIELDS;
    tree_t result;
};


struct label_s {
    TREE_FIELDS;
    toknum_t name;
    tree_t stmt;
};


struct default_s {
    TREE_FIELDS;
    tree_t stmt;
};


struct case_s {
    TREE_FIELDS;
    tree_t expr;
    tree_t stmt;
};


struct constant_s {
    TREE_FIELDS;
    toknum_t value;
};


struct identifier_s {
    TREE_FIELDS;
    toknum_t value;
};


struct special_s {
    TREE_FIELDS;
    toknum_t value;
};


struct accessor_s {
    TREE_FIELDS;
    tree_t left;
    toknum_t op;
    toknum_t field;
};


struct comma_s {
    TREE_FIELDS;
    tree_t *exprs;
};


struct call_s {
    TREE_FIELDS;
    tree_t left;
    tree_t *args;
};


struct cast_s {
    TREE_FIELDS;
    tree_t type_name;
    tree_t expr;
};


struct conditional_s {
    TREE_FIELDS;
    tree_t cond;
    tree_t then_br;
    tree_t else_br;
};


struct subscript_s {
    TREE_FIELDS;
    tree_t left;
    tree_t index;
};


struct unary_s {
    TREE_FIELDS;
    toknum_t op;
    tree_t expr;
};


struct binary_s {
    TREE_FIELDS;
    tree_t left;
    toknum_t op;
    tree_t right;
};


struct assignment_s {
    TREE_FIELDS;
    tree_t left;
    toknum_t op;
    tree_t right;
};


struct comp_literal_s {
    TREE_FIELDS;
    tree_t type_name;
    tree_t *members;
};


struct comp_member_s {
    TREE_FIELDS;
    tree_t *designs;
    tree_t init;
};

#endif  // __TREE_H__
