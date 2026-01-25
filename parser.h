#ifndef PARSER_H
#define PARSER_H

#include "token.h"

enum NodeType {
    AST_PROG,
    AST_LITERAL,
    AST_BINOP,
    AST_UNOP,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
};

struct NcInteger {
    long long int value;
};

struct NcFloat {
    double value;
};

enum ValueType {
    V_NIL,
    V_INT,
    V_FLOAT,
    V_STRING,
};

struct AstValue {
    enum ValueType type;
    union {
        long long int int_value;
        double float_value;
        const char* string_value;
    };
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
    };
};

struct Parser {
    struct Token* tokens;
    struct Token* tok;
};

void draw_ast(struct AstNode* root);
const char* ast_node_to_str(struct AstNode* node);
const char* ast_value_to_str(struct AstValue* value);
const char* binop_type_to_str(enum TokenType binop_type);

struct AstNode* new_node();

void parse(struct Parser* parser, struct AstNode* node);
void parse_atom(struct Parser* parser, struct AstNode* node);
void parse_expr(struct Parser* parser, struct AstNode* node);
void parse_sum(struct Parser* parser, struct AstNode* node);
void parse_term(struct Parser* parser, struct AstNode* node);
void parse_factor(struct Parser* parser, struct AstNode* node);

#endif
