#ifndef PARSER_H
#define PARSER_H

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
    X(AST_FOR)

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
    X(V_CALLABLE)

enum ValueType {
#define X(x) x,
    VALUE_TYPES
#undef X
};

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
        struct {
            void* data;
        };
    };
};

struct AstNode {
    enum NodeType type;
    struct {
        union {
            // AST_LITERAL
            struct {
                struct AstValue value;
            } literal;

            // AST_BINOP
            struct {
                enum TokenType binop_type;
                struct AstNode* lhs;
                struct AstNode* rhs;
            } binop;

            // AST_UNOP
            struct {
                enum TokenType unop_type;
                struct AstNode* node;
            } unop;

            // AST_IDENTIFIER
            struct {
                const char* name;
            } identifier;

            // AST_ASSIGNMENT
            struct {
                struct AstNode* ident;
                struct AstNode* rvalue;
            } assignment;

            // AST_PROGRAM / AST_BLOCK
            struct {
                struct AstNode** stmnts;
                size_t stmnt_count;
            } stmnts;

            // AST_ITEMS
            struct {
                struct AstNode** items;
                size_t item_count;
            } items;

            // AST_FDEF / AST_FDEF
            struct {
                const char* fname;
                struct AstNode** params;
                size_t param_count;

                // AST_FDEF
                struct AstNode* fbody;
            } fcall_or_fdef;

            // AST_IDX
            struct {
                const char* lname;
                struct AstNode* iexpr;
            } idx;

            // AST_FOR
            struct {
                const char* lvar;
                struct AstNode* lexpr;
                struct AstNode* lbody;
            } for_loop;
        };
    } as;
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
