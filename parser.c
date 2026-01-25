/*

program: 'eol'*, stmnts?, 'eol'*
stmnts: stmnt, { end+, stmnt }, end*
stmnt:
  | 'for', 'identifier', 'in', expr, 'eol'?, stmnt
  | 'command', expr*
  | expr, [ 'if', expr ]
expr: disj, [ '..', disj, [ '..', ('+' | '-')?, disj ] ]
disj: conj, { 'or', conj }
conj: neg, { 'and', neg }
neg:
  | 'not', neg
  | comp
comp: sum, { '<' | '>' | '<=' | '>=' | '==' | '!=', sum }
sum: term, { '+' | '-', term }
term: factor, { '*' | '/' | '%', factor }
factor:
  | ('-' | '#'), factor
  | atom, [ '^', factor ]
atom:
  | 'identifier', atom_ident_tail?
  | '(', expr, ')'
  | '[', items?, ']'
  | 'integer'
  | 'float'
  | 'string'
  | 'Inf'
  | block
atom_ident_tail:
  | '=', expr
  | '(', items?, ')', [ '=', expr ]
  | '[', expr, ']', [ '=', expr ]
items: expr, { ',', expr }
block: '{', 'eol'*, stmnts?, 'eol'*, '}'
end: ';' | 'eol'

*/

#include "parser.h"
#include <stdlib.h>
#include "lexer.h"

const char* ast_node_to_str(struct AstNode* node) {
    char* buf = malloc(128);

    switch (node->type) {
        case AST_LITERAL: {
            const char* sval = ast_value_to_str(&node->value);
            sprintf(buf, "(literal %s)", sval);
        } break;
        case AST_BINOP: {
            const char* lhs = ast_node_to_str(node->lhs);
            const char* rhs = ast_node_to_str(node->rhs);
            const char* op = binop_type_to_str(node->binop_type);
            sprintf(buf, "(binop%s %s %s)", op, lhs, rhs);
        } break;
        case AST_UNOP: {
            const char* n = ast_node_to_str(node->node);
            const char* op = binop_type_to_str(node->unop_type);
            sprintf(buf, "(unop%s %s)", op, n);
        } break;
        default:
            fprintf(stderr, "error: unknown AST node type: %d\n", node->type);
            exit(1);
    };

    return buf;
}

const char* ast_value_to_str(struct AstValue* value) {
    char* buf = malloc(128);

    switch (value->type) {
        case V_INT:
            sprintf(buf, "%lld", value->int_value);
            break;
        case V_FLOAT:
            sprintf(buf, "%f", value->float_value);
            break;
        default:
            fprintf(stderr, "error: unknown value type: %d\n", value->type);
            exit(1);
    }

    return buf;
}

const char* binop_type_to_str(enum TokenType binop_type) {
    switch (binop_type) {
        case TOK_LEQ:
            return "<=";
        case TOK_GEQ:
            return ">=";
        case TOK_EEQ:
            return "==";
        case TOK_NEQ:
            return "!=";
        case TOK_PLUS:
            return "+";
        case TOK_MINUS:
            return "-";
        case TOK_STAR:
            return "*";
        case TOK_FSLASH:
            return "/";
        case TOK_POWER:
            return "^";
        case TOK_PERC:
            return "%";
        case TOK_LT:
            return "<";
        case TOK_GT:
            return ">";
        default:
            fprintf(stderr, "error: unknown binop type: %d\n", binop_type);
            exit(1);
    }
}

static struct AstNode* new_node() {
    return malloc(sizeof(struct AstNode));
}

void syntax_error(const char* msg) {
    fprintf(stderr, "syntax_error: %s\n", msg);
}

void parse_expr(struct Parser* parser, struct AstNode* node) {
    parse_sum(parser, node);
}

void parse_sum(struct Parser* parser, struct AstNode* node) {
    parse_term(parser, node);

    struct AstNode* tmp = new_node();
    while (parser->tok->type == TOK_PLUS || parser->tok->type == TOK_MINUS) {
        enum TokenType op = parser->tok->type;
        parser->tok++;

        struct AstNode* rhs = new_node();
        parse_term(parser, rhs);

        struct AstNode* lhs = new_node();
        *lhs = *node;

        node->type = AST_BINOP;
        node->binop_type = op;
        node->lhs = lhs;
        node->rhs = rhs;
    }
}

void parse_term(struct Parser* parser, struct AstNode* node) {
    parse_factor(parser, node);

    struct AstNode* tmp = new_node();
    while (parser->tok->type == TOK_STAR || parser->tok->type == TOK_FSLASH || parser->tok->type == TOK_PERC) {
        enum TokenType op = parser->tok->type;
        parser->tok++;

        struct AstNode* rhs = new_node();
        parse_factor(parser, rhs);

        struct AstNode* lhs = new_node();
        *lhs = *node;

        node->type = AST_BINOP;
        node->binop_type = op;
        node->lhs = lhs;
        node->rhs = rhs;
    }
}

void parse_factor(struct Parser* parser, struct AstNode* node) {
    if (parser->tok->type == TOK_MINUS || parser->tok->type == TOK_HASH) {
        node->type = AST_UNOP;
        node->unop_type = parser->tok->type;
        parser->tok++;
        node->node = new_node();
        parse_factor(parser, node->node);

        return;
    }

    parse_atom(parser, node);

    if (parser->tok->type == TOK_POWER) {
        struct AstNode* lhs = new_node();
        *lhs = *node;
        node->type = AST_BINOP;
        node->binop_type = parser->tok->type;
        node->lhs = lhs;
        parser->tok++;
        node->rhs = new_node();
        parse_factor(parser, node->rhs);
    }
}

void parse_atom(struct Parser* parser, struct AstNode* node) {
    switch (parser->tok->type) {
        case TOK_INTEGER: {
            node->type = AST_LITERAL;
            node->value.type = V_INT;
            node->value.int_value = atoll(parser->tok->value);
            parser->tok++;
        } break;
        case TOK_FLOAT: {
            node->type = AST_LITERAL;
            node->value.type = V_FLOAT;
            node->value.float_value = atof(parser->tok->value);
            parser->tok++;
        } break;
        default:
            fprintf(stderr, "syntax_error: unexpected token: %s", tok_to_str(*parser->tok));
            exit(1);
    };
}

void parse(struct Parser* parser, struct AstNode* node) {
    parse_expr(parser, node);
}
