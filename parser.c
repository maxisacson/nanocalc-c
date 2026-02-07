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

#define expect3(tt1, tt2, tt3)                                                                              \
    if (parser->tok->type != (tt1) && parser->tok->type != (tt2) && parser->tok->type != (tt3)) {           \
        syntax_error("expected %s, %s, or %s but got %s\n", tok_type_to_str((tt1)), tok_type_to_str((tt2)), \
                     tok_type_to_str((tt3)), tok_type_to_str(parser->tok->type));                           \
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
                fprintf(out, "v_%p[label=\"%s\"]\n", n, ast_value_to_str(&n->as.literal.value));
                break;
            case AST_BINOP: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, binop_type_to_str(n->as.binop.binop_type));
                fprintf(out, "v_%p -- v_%p\n", n, n->as.binop.lhs);
                fprintf(out, "v_%p -- v_%p\n", n, n->as.binop.rhs);
                queue[i++] = n->as.binop.lhs;
                queue[i++] = n->as.binop.rhs;
            } break;
            case AST_IDENTIFIER: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, n->as.identifier.name);
            } break;
            case AST_ASSIGNMENT: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "=");
                fprintf(out, "v_%p -- v_%p\n", n, n->as.assignment.ident);
                fprintf(out, "v_%p -- v_%p\n", n, n->as.assignment.rvalue);
                queue[i++] = n->as.assignment.ident;
                queue[i++] = n->as.assignment.rvalue;
            } break;
            case AST_PROGRAM: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "program");
                for (size_t j = 0; j < n->as.stmnts.stmnt_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->as.stmnts.stmnts[j]);
                    queue[i++] = n->as.stmnts.stmnts[j];
                }
            } break;
            case AST_ITEMS: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "items");
                for (size_t j = 0; j < n->as.items.item_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->as.items.items[j]);
                    queue[i++] = n->as.items.items[j];
                }
            } break;
            case AST_FCALL: {
                fprintf(out, "v_%p[label=\"%s()\"]\n", n, n->as.fcall_or_fdef.fname);
                for (size_t j = 0; j < n->as.fcall_or_fdef.param_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->as.fcall_or_fdef.params[j]);
                    queue[i++] = n->as.fcall_or_fdef.params[j];
                }
            } break;
            case AST_FDEF: {
                fprintf(out, "v_%p[label=\"%s(", n, n->as.fcall_or_fdef.fname);
                for (size_t j = 0; j < n->as.fcall_or_fdef.param_count; ++j) {
                    if (j > 0) {
                        fprintf(out, ", ");
                    }
                    fprintf(out, "%s", n->as.fcall_or_fdef.params[j]->as.identifier.name);
                }
                fprintf(out, ")\"]\n");
                fprintf(out, "v_%p -- v_%p\n", n, n->as.fcall_or_fdef.fbody);
                queue[i++] = n->as.fcall_or_fdef.fbody;
            } break;
            case AST_BLOCK: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "block");
                for (size_t j = 0; j < n->as.stmnts.stmnt_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->as.stmnts.stmnts[j]);
                    queue[i++] = n->as.stmnts.stmnts[j];
                }
            } break;
            case AST_FOR: {
                fprintf(out, "v_%p[label=\"%s %s\"]\n", n, "for", n->as.for_loop.lvar);
                fprintf(out, "v_%p -- v_%p\n", n, n->as.for_loop.lexpr);
                fprintf(out, "v_%p -- v_%p\n", n, n->as.for_loop.lbody);
                queue[i++] = n->as.for_loop.lexpr;
                queue[i++] = n->as.for_loop.lbody;
            } break;
            default:
                error("%s: unknown AST node type: %s\n", __PRETTY_FUNCTION__, node_type_to_str(n->type));
        };
    }

    fprintf(out, "}\n");
    fclose(out);

    system("dot -Tsvg -oast.svg ast.dot");
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

struct AstNode* node_new() {
    node_count++;
    return malloc(sizeof(struct AstNode));
}

void parse(struct Parser* parser, struct AstNode* root) {
    parse_program(parser, root);
}

void parse_program(struct Parser* parser, struct AstNode* node) {
    while (parser->tok->type == TOK_EOL) {
        parser->tok++;
    }
    node->type = AST_PROGRAM;

    PtrArr arr = {};
    struct AstNode* tmp;

    while (parser->tok->type != TOK_EOF) {
        tmp = node_new();
        parse_stmnt(parser, tmp);
        ptrarr_append(&arr, tmp);

        if (parser->tok->type != TOK_EOF) {
            expect2(TOK_SEMICOLON, TOK_EOL);
        }

        while (parser->tok->type == TOK_SEMICOLON || parser->tok->type == TOK_EOL) {
            parser->tok++;
        }
    }

    node->as.stmnts.stmnt_count = arr.size;
    node->as.stmnts.stmnts = (struct AstNode**)arr.data;

    while (parser->tok->type == TOK_EOL) {
        parser->tok++;
    }
}

void parse_stmnt(struct Parser* parser, struct AstNode* node) {
    if (parser->tok->type == KW_for) {
        node->type = AST_FOR;
        parser->tok++;

        expect(TOK_IDENTIFIER);
        node->as.for_loop.lvar = parser->tok->value;
        parser->tok++;

        expect(KW_in);
        parser->tok++;

        node->as.for_loop.lexpr = node_new();
        parse_expr(parser, node->as.for_loop.lexpr);

        if (parser->tok->type == TOK_EOL) {
            parser->tok++;
        }

        node->as.for_loop.lbody = node_new();
        parse_stmnt(parser, node->as.for_loop.lbody);
    } else {
        parse_expr(parser, node);
    }
}

void parse_expr(struct Parser* parser, struct AstNode* node) {
    parse_sum(parser, node);
}

void parse_sum(struct Parser* parser, struct AstNode* node) {
    parse_term(parser, node);

    while (parser->tok->type == TOK_PLUS || parser->tok->type == TOK_MINUS) {
        enum TokenType op = parser->tok->type;
        parser->tok++;

        struct AstNode* rhs = node_new();
        parse_term(parser, rhs);

        struct AstNode* lhs = node_new();
        *lhs = *node;

        node->type = AST_BINOP;
        node->as.binop.binop_type = op;
        node->as.binop.lhs = lhs;
        node->as.binop.rhs = rhs;
    }
}

void parse_term(struct Parser* parser, struct AstNode* node) {
    parse_factor(parser, node);

    while (parser->tok->type == TOK_STAR || parser->tok->type == TOK_FSLASH || parser->tok->type == TOK_PERC) {
        enum TokenType op = parser->tok->type;
        parser->tok++;

        struct AstNode* rhs = node_new();
        parse_factor(parser, rhs);

        struct AstNode* lhs = node_new();
        *lhs = *node;

        node->type = AST_BINOP;
        node->as.binop.binop_type = op;
        node->as.binop.lhs = lhs;
        node->as.binop.rhs = rhs;
    }
}

void parse_factor(struct Parser* parser, struct AstNode* node) {
    if (parser->tok->type == TOK_MINUS || parser->tok->type == TOK_HASH) {
        node->type = AST_UNOP;
        node->as.unop.unop_type = parser->tok->type;
        parser->tok++;
        node->as.unop.node = node_new();
        parse_factor(parser, node->as.unop.node);

        return;
    }

    parse_atom(parser, node);

    if (parser->tok->type == TOK_POWER) {
        struct AstNode* lhs = node_new();
        *lhs = *node;
        node->type = AST_BINOP;
        node->as.binop.binop_type = parser->tok->type;
        node->as.binop.lhs = lhs;
        parser->tok++;
        node->as.binop.rhs = node_new();
        parse_factor(parser, node->as.binop.rhs);
    }
}

void parse_atom_ident_tail(struct Parser* parser, struct AstNode* node) {
    switch (parser->tok->type) {
        case TOK_EQ: {
            node->type = AST_ASSIGNMENT;
            Node_t* lhs = node_new();
            *lhs = *node;
            node->as.assignment.ident = lhs;
            parser->tok++;
            node->as.assignment.rvalue = node_new();
            parse_expr(parser, node->as.assignment.rvalue);
        } break;
        case TOK_LPAREN: {
            parser->tok++;

            node->type = AST_FCALL;
            node->as.fcall_or_fdef.fname = node->as.identifier.name;

            if (parser->tok->type != TOK_RPAREN) {
                struct AstNode* tmp = node_new();
                tmp->as.items.item_count = 0;
                parse_items(parser, tmp);
                node->as.fcall_or_fdef.params = tmp->as.items.items;
                node->as.fcall_or_fdef.param_count = tmp->as.items.item_count;
            }

            expect(TOK_RPAREN);
            parser->tok++;

            if (parser->tok->type == TOK_EQ) {
                parser->tok++;

                node->type = AST_FDEF;
                for (size_t ip = 0; ip < node->as.fcall_or_fdef.param_count; ++ip) {
                    if (node->as.fcall_or_fdef.params[ip]->type != AST_IDENTIFIER) {
                        syntax_error("expected identifier but got %s\n",
                                     node_type_to_str(node->as.fcall_or_fdef.params[ip]->type));
                    }
                }
                node->as.fcall_or_fdef.fbody = node_new();
                parse_expr(parser, node->as.fcall_or_fdef.fbody);
            }
        } break;
        case TOK_LBRACKET: {
            parser->tok++;

            node->type = AST_IDX;
            node->as.idx.lname = node->as.identifier.name;
            node->as.idx.iexpr = node_new();

            parse_expr(parser, node->as.idx.iexpr);
            expect(TOK_RBRACKET);
            parser->tok++;

            if (parser->tok->type == TOK_EQ) {
                parser->tok++;

                struct AstNode* lhs = node_new();
                *lhs = *node;
                node->type = AST_ASSIGNMENT;
                node->as.assignment.ident = lhs;
                node->as.assignment.rvalue = node_new();
                parse_expr(parser, node->as.assignment.rvalue);
            }
        } break;
        default:
            break;
    }
}

void parse_items(struct Parser* parser, struct AstNode* node) {
    node->type = AST_ITEMS;

    PtrArr arr = {};

    struct AstNode* tmp = node_new();
    parse_expr(parser, tmp);
    ptrarr_append(&arr, tmp);

    while (parser->tok->type == TOK_COMMA) {
        parser->tok++;
        tmp = node_new();
        parse_expr(parser, tmp);
        ptrarr_append(&arr, tmp);
    }
    node->as.items.items = (struct AstNode**)arr.data;
    node->as.items.item_count = arr.size;
}

void parse_block(struct Parser* parser, struct AstNode* node) {
    expect(TOK_LBRACE);
    parser->tok++;
    node->type = AST_BLOCK;

    while (parser->tok->type == TOK_EOL) {
        parser->tok++;
    }

    PtrArr arr = {};

    struct AstNode* tmp;

    while (parser->tok->type != TOK_RBRACE) {
        tmp = node_new();
        parse_stmnt(parser, tmp);
        ptrarr_append(&arr, tmp);

        expect3(TOK_RBRACE, TOK_SEMICOLON, TOK_EOL);

        while (parser->tok->type == TOK_SEMICOLON || parser->tok->type == TOK_EOL) {
            parser->tok++;
        }
    }

    node->as.stmnts.stmnts = (struct AstNode**)arr.data;
    node->as.stmnts.stmnt_count = arr.size;

    while (parser->tok->type == TOK_EOL) {
        parser->tok++;
    }

    expect(TOK_RBRACE);
    parser->tok++;
}

void parse_atom(struct Parser* parser, struct AstNode* node) {
    switch (parser->tok->type) {
        case TOK_IDENTIFIER: {
            node->type = AST_IDENTIFIER;
            node->as.identifier.name = parser->tok->value;
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
            node->as.literal.value.type = V_INT;
            node->as.literal.value.int_value = atoll(parser->tok->value);
            parser->tok++;
        } break;
        case TOK_FLOAT: {
            node->type = AST_LITERAL;
            node->as.literal.value.type = V_FLOAT;
            node->as.literal.value.float_value = atof(parser->tok->value);
            parser->tok++;
        } break;
        case TOK_STRING: {
            node->type = AST_LITERAL;
            node->as.literal.value.type = V_STRING;
            node->as.literal.value.string_value = parser->tok->value;
            parser->tok++;
        } break;
        case TOK_LBRACE: {
            parse_block(parser, node);
        } break;
        default:
            syntax_error("unexpected token: %s\n", tok_to_str(*parser->tok));
    };
}
