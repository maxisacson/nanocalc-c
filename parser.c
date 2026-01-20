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

const char* ast_node_to_str(struct AstNode node) {
    char* buf = malloc(128);

    switch (node.type) {
        case AST_LITERAL:
            switch (node.value.type) {
                case V_INT:
                    sprintf(buf, "(literal %lld)", node.value.int_value);
                    break;
                case V_FLOAT:
                    sprintf(buf, "(literal %f)", node.value.float_value);
                    break;
                default:
                    fprintf(stderr, "error: unknown value type: %d\n", node.value.type);
                    exit(1);
            }
            break;
        case AST_BINOP: {
            const char* lhs = ast_node_to_str(*node.lhs);
            const char* rhs = ast_node_to_str(*node.rhs);
            sprintf(buf, "(binop %s %s)", lhs, rhs);
        }
        break;
        default:
            fprintf(stderr, "error: unknown AST node type: %d\n", node.type);
            exit(1);
    };

    return buf;
}

static struct AstNode* new_node() {
    return malloc(sizeof(struct AstNode));
}

void syntax_error(const char* msg) {
    fprintf(stderr, "syntax_error: %s\n", msg);
}

void parse_expr(struct Parser* parser, struct AstNode* node) {
    struct AstNode* lhs = new_node();
    struct AstNode* rhs = new_node();

    parse_atom(parser, lhs);
    node->lhs = lhs;

    if (parser->tok->type != TOK_PLUS && parser->tok->type != TOK_MINUS) {
        syntax_error("expected binary operator");
        exit(EXIT_FAILURE);
    }

    node->type = AST_BINOP;
    node->binop_type = (parser->tok++)->type;

    parse_atom(parser, rhs);
    node->rhs = rhs;
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
