#ifndef PARSER_H
#define PARSER_H

#include "token.h"

enum NodeType {
    AST_PROG,
    AST_LITERAL,
    AST_BINOP,
    AST_UNOP,
};

struct NcInteger {
    long long int value;
};

struct NcFloat {
    double value;
};

enum ValueType {
    V_INT,
    V_FLOAT,
};

struct AstValue {
    enum ValueType type;
    union {
        long long int int_value;
        double float_value;
    };
};

struct AstNode {
    enum NodeType type;
    union {
        struct AstValue value;
        struct {
            enum TokenType binop_type;
            struct AstNode* lhs;
            struct AstNode* rhs;
        };
        struct {
            enum TokenType unop_type;
            struct AstNode* node;
        };
    };
};

struct Parser {
    struct Token* tokens;
    struct Token* tok;
};

const char* ast_node_to_str(struct AstNode* node);
const char* ast_value_to_str(struct AstValue* value);
const char* binop_type_to_str(enum TokenType binop_type);

void parse(struct Parser* parser, struct AstNode* node);
void parse_atom(struct Parser* parser, struct AstNode* node);
void parse_expr(struct Parser* parser, struct AstNode* node);
void parse_sum(struct Parser* parser, struct AstNode* node);
void parse_term(struct Parser* parser, struct AstNode* node);
void parse_factor(struct Parser* parser, struct AstNode* node);

#endif
