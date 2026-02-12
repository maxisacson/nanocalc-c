#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "token.h"

#define NODE_TYPES    \
    X(AST_PROG)       \
    X(AST_LITERAL)    \
    X(AST_BINOP)      \
    X(AST_UNOP)       \
    X(AST_IDENTIFIER) \
    X(AST_ASSIGNMENT) \
    X(AST_PROGRAM)    \
    X(AST_ITEMS)      \
    X(AST_FCALL)      \
    X(AST_FDEF)       \
    X(AST_IDX)        \
    X(AST_BLOCK)      \
    X(AST_FOR)        \
    X(AST_RANGE)      \
    X(AST_CMD)

enum NodeType {
#define X(x) x,
    NODE_TYPES
#undef X
};

#define VALUE_TYPES \
    X(V_NIL)        \
    X(V_INT)        \
    X(V_FLOAT)      \
    X(V_STRING)     \
    X(V_LIST)       \
    X(V_RANGE)      \
    X(V_CALLABLE)

enum ValueType {
#define X(x) x,
    VALUE_TYPES
#undef X
};

struct RangeValue;

struct AstValue {
    enum ValueType type;
    union {
        // V_INT
        long long int int_value;

        // V_FLOAT
        double float_value;

        // V_STRING
        const char* string_value;

        // V_LIST
        struct {
            struct AstValue* list_value;
            size_t list_size;
        };

        // V_CALLABLE
        void* data;

        // V_RANGE
        struct RangeValue* range_value;
    };
};

struct RangeValue {
    struct AstValue start;
    struct AstValue stop;
    struct AstValue value;
    struct AstValue step;
    size_t count;
    size_t length;
    bool done;
    bool started;
};

struct AstNode {
    enum NodeType type;
    union {
        // AST_LITERAL
        struct AstValue value;

        // AST_BINOP
        struct {
            enum TokenType binop_type;
            struct AstNode* lhs;
            struct AstNode* rhs;
        };

        // AST_UNOP
        struct {
            enum TokenType unop_type;
            struct AstNode* node;
        };

        // AST_IDENTIFIER
        struct {
            const char* name;
        };

        // AST_ASSIGNMENT
        struct {
            struct AstNode* ident;
            struct AstNode* rvalue;
        };

        // AST_PROGRAM / AST_BLOCK
        struct {
            struct AstNode** stmnts;
            size_t stmnt_count;
        };

        // AST_ITEMS
        struct {
            struct AstNode** items;
            size_t item_count;
        };

        // AST_FCALL / AST_FDEF
        struct {
            const char* fname;
            struct AstNode** params;
            size_t param_count;

            // AST_FDEF
            struct AstNode* fbody;
        };

        // AST_IDX
        struct {
            const char* lname;
            struct AstNode* iexpr;
        };

        // AST_FOR
        struct {
            const char* lvar;
            struct AstNode* lexpr;
            struct AstNode* lbody;
        };

        // AST_RANGE
        struct {
            struct AstNode* rstart;
            struct AstNode* rstop;
            struct AstNode* rcount;
            struct AstNode* rstep;
        };

        // AST_CMD
        struct {
            const char* cmd;
            struct AstNode** cargs;
            size_t carg_count;
        };
    };
};

struct Parser {
    struct Token* tokens;
    struct Token* tok;
};

void draw_ast(struct AstNode* root);
const char* ast_value_to_str(struct AstValue* value);
const char* node_type_to_str(enum NodeType node_type);
const char* binop_type_to_str(enum TokenType binop_type);
const char* value_type_to_str(enum ValueType value_type);

void parse(struct Parser* parser, struct AstNode* node);

void parse_program(struct Parser*, struct AstNode* node);
void parse_stmnt(struct Parser*, struct AstNode* node);
void parse_expr(struct Parser* parser, struct AstNode* node);
void parse_sum(struct Parser* parser, struct AstNode* node);
void parse_term(struct Parser* parser, struct AstNode* node);
void parse_factor(struct Parser* parser, struct AstNode* node);
void parse_atom(struct Parser* parser, struct AstNode* node);
void parse_items(struct Parser* parser, struct AstNode* node);

#endif
