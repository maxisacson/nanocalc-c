#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "token.h"
#include "nc.h"

#define NODE_TYPES    \
    X(AST_UNDEFINED)  \
    X(AST_PROGRAM)    \
    X(AST_LITERAL)    \
    X(AST_BINOP)      \
    X(AST_UNOP)       \
    X(AST_IDENTIFIER) \
    X(AST_ASSIGNMENT) \
    X(AST_ITEMS)      \
    X(AST_FCALL)      \
    X(AST_FDEF)       \
    X(AST_IDX)        \
    X(AST_BLOCK)      \
    X(AST_FOR)        \
    X(AST_RANGE)      \
    X(AST_CASE)       \
    X(AST_CASES)      \
    X(AST_CMD)

enum NodeType {
#define X(x) x,
    NODE_TYPES
#undef X
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

        // AST_PROGRAM / AST_BLOCK / AST_CASES
        struct {
            size_t stmnt_count;
            struct AstNode** stmnts;
        };

        // AST_ITEMS
        struct {
            size_t item_count;
            struct AstNode** items;
        };

        // AST_FCALL / AST_FDEF
        struct {
            const char* fname;
            size_t param_count;
            struct AstNode** params;

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
            size_t carg_count;
            struct AstNode** cargs;
        };

        // AST_CASE
        struct {
            struct AstNode* cexpr;
            struct AstNode* pred;
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
const char* unop_type_to_str(enum TokenType unop_type);
const char* value_type_to_str(enum ValueType value_type);

void parse(struct Parser* parser, struct AstNode* node);

void parse_program(struct Parser* parser, struct AstNode* node);
void parse_stmnt(struct Parser* parser, struct AstNode* node);
void parse_expr(struct Parser* parser, struct AstNode* node);
void parse_disj(struct Parser* parser, struct AstNode* node);
void parse_conj(struct Parser* parser, struct AstNode* node);
void parse_neg(struct Parser* parser, struct AstNode* node);
void parse_comp(struct Parser* parser, struct AstNode* node);
void parse_range(struct Parser* parser, struct AstNode* node);
void parse_sum(struct Parser* parser, struct AstNode* node);
void parse_term(struct Parser* parser, struct AstNode* node);
void parse_factor(struct Parser* parser, struct AstNode* node);
void parse_atom(struct Parser* parser, struct AstNode* node);
void parse_items(struct Parser* parser, struct AstNode* node);

#endif

// vim: ft=c
