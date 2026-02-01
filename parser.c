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
#include <string.h>
#include "lexer.h"
#include "utils.h"

typedef struct AstNode Node_t;

size_t node_count = 0;

#define expect(token_type)                                                                                           \
    if (parser->tok->type != (token_type)) {                                                                         \
        syntax_error("expected %s but got %s\n", tok_type_to_str((token_type)), tok_type_to_str(parser->tok->type)); \
    }

#define expect2(token_type1, token_type2)                                                                              \
    if (parser->tok->type != (token_type1) && parser->tok->type != (token_type2)) {                                    \
        syntax_error("expected %s or %s but got %s\n", tok_type_to_str((token_type1)), tok_type_to_str((token_type2)), \
                     tok_type_to_str(parser->tok->type));                                                              \
    }

void draw_ast(Node_t* root) {
    Node_t** queue = malloc(node_count * sizeof(Node_t*));

    size_t i = 0;
    queue[i++] = root;

    FILE* out = fopen("ast.dot", "w");
    fprintf(out, "graph {\n");

    Node_t* n;
    while (i > 0) {
        n = queue[--i];
        switch (n->type) {
            case AST_LITERAL:
                fprintf(out, "v_%p[label=\"%s\"]\n", n, ast_value_to_str(&n->value));
                break;
            case AST_BINOP: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, binop_type_to_str(n->binop_type));
                fprintf(out, "v_%p -- v_%p\n", n, n->lhs);
                fprintf(out, "v_%p -- v_%p\n", n, n->rhs);
                queue[i++] = n->lhs;
                queue[i++] = n->rhs;
            } break;
            case AST_IDENTIFIER: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, n->name);
            } break;
            case AST_ASSIGNMENT: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "=");
                fprintf(out, "v_%p -- v_%p\n", n, n->ident);
                fprintf(out, "v_%p -- v_%p\n", n, n->rvalue);
                queue[i++] = n->ident;
                queue[i++] = n->rvalue;
            } break;
            case AST_PROGRAM: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "program");
                for (size_t j = 0; j < n->stmnt_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->stmnts[j]);
                    queue[i++] = n->stmnts[j];
                }
            } break;
            case AST_ITEMS: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "items");
                for (size_t j = 0; j < n->item_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->items[j]);
                    queue[i++] = n->items[j];
                }
            } break;
            case AST_FCALL: {
                fprintf(out, "v_%p[label=\"%s()\"]\n", n, n->fname);
                for (size_t j = 0; j < n->params->item_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->params->items[j]);
                    queue[i++] = n->params->items[j];
                }
            } break;
            default:
                error("%s: unknown AST node type: %s\n", __PRETTY_FUNCTION__, node_type_to_str(n->type));
        };
    }

    fprintf(out, "}\n");
    fclose(out);

    system("dot -Tsvg -oast.svg ast.dot");
}

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
        case AST_IDENTIFIER: {
            const char* name = node->name;
            sprintf(buf, "(identifier %s)", name);
        } break;
        case AST_ASSIGNMENT: {
            const char* ident = ast_node_to_str(node->ident);
            const char* rvalue = ast_node_to_str(node->rvalue);
            sprintf(buf, "(= %s %s)", ident, rvalue);
        } break;
        case AST_PROGRAM: {
            sprintf(buf, "(program");
            for (size_t i = 0; i < node->stmnt_count; ++i) {
                sprintf(buf, "%s %s", buf, ast_node_to_str(node->stmnts[i]));
            }
            sprintf(buf, "%s)", buf);
        } break;
        case AST_ITEMS: {
            sprintf(buf, "(items");
            for (size_t i = 0; i < node->item_count; ++i) {
                sprintf(buf, "%s %s", buf, ast_node_to_str(node->items[i]));
            }
            sprintf(buf, "%s)", buf);
        } break;
        case AST_FCALL: {
            sprintf(buf, "(%s", node->fname);
            for (size_t i = 0; i < node->params->item_count; ++i) {
                sprintf(buf, "%s %s", buf, ast_node_to_str(node->params->items[i]));
            }
            sprintf(buf, "%s)", buf);
        } break;
        default:
            error("%s: unknown AST node type: %s\n", __PRETTY_FUNCTION__, node_type_to_str(node->type));
    };

    return buf;
}

const char* node_type_to_str(enum NodeType node_type) {
    switch (node_type) {
#define X(x) \
    case x:  \
        return #x;
        NODE_TYPES
#undef X
        default:
            fprintf(stderr, "error: unknown token type: %d\n", node_type);
            exit(1);
    }
}

const char* value_type_to_str(enum ValueType value_type) {
    switch (value_type) {
#define X(x) \
    case x:  \
        return #x;
        VALUE_TYPES
#undef X
        default:
            error("unknown value type: %d\n", value_type);
    }
}

const char* ast_value_to_str(struct AstValue* value) {
    String s = {};
    string_reserve(&s, 128);

    switch (value->type) {
        case V_NIL:
            sprintf(s.data, "nil");
            break;
        case V_INT:
            sprintf(s.data, "%lld", value->int_value);
            break;
        case V_FLOAT:
            sprintf(s.data, "%f", value->float_value);
            break;
        case V_STRING: {
            string_set(&s, value->string_value);
        } break;
        case V_LIST: {
            string_set(&s, "[");
            for (size_t i = 0; i < value->list_size; ++i) {
                if (i > 0) {
                    string_append(&s, ", ");
                }
                string_append(&s, ast_value_to_str(value->list_value + i));
            }
            string_append(&s, "]");
        } break;
        default:
            fprintf(stderr, "error: %s: unknown value type: %d\n", __PRETTY_FUNCTION__, value->type);
            exit(1);
    }

    return s.data;
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

struct AstNode* new_node() {
    struct AstNode* node = malloc(sizeof(struct AstNode));
    return node;
}

void parse(struct Parser* parser, struct AstNode* node) {
    parse_program(parser, node);
}

void parse_program(struct Parser* parser, struct AstNode* node) {
    while (parser->tok->type == TOK_EOL) {
        parser->tok++;
    }
    node->type = AST_PROGRAM;

    size_t stmnts_cap = 128;
    node->stmnt_count = 0;
    node->stmnts = malloc(stmnts_cap * sizeof(Node_t*));

    while (parser->tok->type != TOK_EOF) {
        if (node->stmnt_count >= stmnts_cap) {
            stmnts_cap *= 2;
            node->stmnts = realloc(node->stmnts, stmnts_cap * sizeof(Node_t*));
        }
        node->stmnts[node->stmnt_count] = malloc(sizeof(Node_t));
        parse_stmnt(parser, node->stmnts[node->stmnt_count]);
        node->stmnt_count++;

        if (parser->tok->type != TOK_EOF) {
            expect2(TOK_SEMICOLON, TOK_EOL);
        }

        while (parser->tok->type == TOK_SEMICOLON || parser->tok->type == TOK_EOL) {
            parser->tok++;
        }
    }

    while (parser->tok->type == TOK_EOL) {
        parser->tok++;
    }
}

void parse_stmnt(struct Parser* parser, struct AstNode* node) {
    parse_expr(parser, node);
}

void parse_expr(struct Parser* parser, struct AstNode* node) {
    parse_sum(parser, node);
}

void parse_sum(struct Parser* parser, struct AstNode* node) {
    parse_term(parser, node);

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

void parse_atom_ident_tail(struct Parser* parser, struct AstNode* node) {
    switch (parser->tok->type) {
        case TOK_EQ: {
            Node_t* lhs = new_node();
            *lhs = *node;
            node->ident = lhs;
            node->type = AST_ASSIGNMENT;
            parser->tok++;
            node->rvalue = new_node();
            parse_expr(parser, node->rvalue);
        } break;
        case TOK_LPAREN: {
            parser->tok++;

            node->type = AST_FCALL;
            node->fname = node->name;
            node->params = new_node();
            node->params->item_count = 0;

            if (parser->tok->type != TOK_RPAREN) {
                parse_items(parser, node->params);
            }

            expect(TOK_RPAREN);
            parser->tok++;

            if (parser->tok->type == TOK_EQ) {
                parser->tok++;

                node->type = AST_FDEF;
                for (size_t ip = 0; ip < node->params->item_count; ++ip) {
                    if (node->params->items[ip]->type != AST_IDENTIFIER) {
                        syntax_error("expected identifier but got %s\n",
                                     node_type_to_str(node->params->items[ip]->type));
                    }
                }
                node->fbody = new_node();
                parse_expr(parser, node->fbody);
            }
        } break;
        case TOK_LBRACKET: {
            parser->tok++;

            node->type = AST_IDX;
            node->lname = node->name;
            node->iexpr = new_node();

            parse_expr(parser, node->iexpr);
            expect(TOK_RBRACKET);
            parser->tok++;

            if (parser->tok->type == TOK_EQ) {
                parser->tok++;

                struct AstNode* tmp = new_node();
                *tmp = *node;
                node->type = AST_ASSIGNMENT;
                node->ident = tmp;
                node->rvalue = new_node();
                parse_expr(parser, node->rvalue);
            }
        } break;
        default:
            break;
    }
}

void parse_items(struct Parser* parser, struct AstNode* node) {
    node->type = AST_ITEMS;
    size_t cap = 128;
    node->items = malloc(cap * sizeof(Node_t*));
    node->item_count = 0;

    node->items[node->item_count] = malloc(sizeof(Node_t));
    parse_expr(parser, node->items[node->item_count]);
    node->item_count++;

    while (parser->tok->type == TOK_COMMA) {
        parser->tok++;

        if (node->item_count >= cap) {
            cap *= 2;
            node->items = realloc(node->items, cap * sizeof(Node_t*));
        }

        node->items[node->item_count] = malloc(sizeof(Node_t));
        parse_expr(parser, node->items[node->item_count]);
        node->item_count++;
    }
}

void parse_atom(struct Parser* parser, struct AstNode* node) {
    switch (parser->tok->type) {
        case TOK_IDENTIFIER: {
            node->type = AST_IDENTIFIER;
            node->name = parser->tok->value;
            parser->tok++;
            parse_atom_ident_tail(parser, node);
        } break;
        case TOK_LPAREN: {
            parser->tok++;
            parse_expr(parser, node);
            expect(TOK_RPAREN);
            parser->tok++;
        } break;
        case TOK_LBRACKET: {
            parser->tok++;
            parse_items(parser, node);
            expect(TOK_RBRACKET);
            parser->tok++;
        } break;
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
        case TOK_STRING: {
            node->type = AST_LITERAL;
            node->value.type = V_STRING;
            node->value.string_value = parser->tok->value;
            parser->tok++;
        } break;
        default:
            syntax_error("unexpected token: %s\n", tok_to_str(*parser->tok));
    };
}
