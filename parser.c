/*

program: 'eol'*, stmnts?, 'eol'*
stmnts: stmnt, { end+, stmnt }, end*
stmnt:
  | 'for', 'identifier', 'in', expr, 'eol'?, stmnt
  | 'command', expr*
  | expr, [ 'if', expr ]
expr: disj
disj: conj, { '|', conj }
conj: comp, { '&', comp }
comp: range, { '<' | '>' | '<=' | '>=' | '==' | '!=', range }
range: sum, [ '..', sum, [ '..', '+'?, sum ] ]
sum: term, { '+' | '-', term }
term: factor, { '*' | '/' | '%', factor }
factor:
  | ('-' | '#', '!'), factor
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
                fprintf(out, "v_%p[label=\"%s\"]\n", n, ast_value_to_str(&n->value));
                break;
            case AST_BINOP: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, binop_type_to_str(n->binop_type));
                fprintf(out, "v_%p -- v_%p\n", n, n->lhs);
                fprintf(out, "v_%p -- v_%p\n", n, n->rhs);
                queue[i++] = n->lhs;
                queue[i++] = n->rhs;
            } break;
            case AST_UNOP: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, unop_type_to_str(n->unop_type));
                fprintf(out, "v_%p -- v_%p\n", n, n->node);
                queue[i++] = n->node;
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
                for (size_t j = 0; j < n->param_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->params[j]);
                    queue[i++] = n->params[j];
                }
            } break;
            case AST_FDEF: {
                fprintf(out, "v_%p[label=\"%s(", n, n->fname);
                for (size_t j = 0; j < n->param_count; ++j) {
                    if (j > 0) {
                        fprintf(out, ", ");
                    }
                    fprintf(out, "%s", n->params[j]->name);
                }
                fprintf(out, ")\"]\n");
                fprintf(out, "v_%p -- v_%p\n", n, n->fbody);
                queue[i++] = n->fbody;
            } break;
            case AST_BLOCK: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "block");
                for (size_t j = 0; j < n->stmnt_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->stmnts[j]);
                    queue[i++] = n->stmnts[j];
                }
            } break;
            case AST_FOR: {
                fprintf(out, "v_%p[label=\"%s %s\"]\n", n, "for", n->lvar);
                fprintf(out, "v_%p -- v_%p [label=\"%s\"]\n", n, n->lexpr, "in");
                fprintf(out, "v_%p -- v_%p\n", n, n->lbody);
                queue[i++] = n->lexpr;
                queue[i++] = n->lbody;
            } break;
            case AST_RANGE: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "range");
                fprintf(out, "v_%p -- v_%p [label=\"%s\"]\n", n, n->rstart, "start");
                fprintf(out, "v_%p -- v_%p [label=\"%s\"]\n", n, n->rstop, "stop");
                queue[i++] = n->rstart;
                queue[i++] = n->rstop;
                if (n->rcount) {
                    fprintf(out, "v_%p -- v_%p [label=\"%s\"]\n", n, n->rcount, "count");
                    queue[i++] = n->rcount;
                } else if (n->rstep) {
                    fprintf(out, "v_%p -- v_%p [label=\"%s\"]\n", n, n->rstep, "step");
                    queue[i++] = n->rstep;
                }
            } break;
            case AST_CMD: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, n->cmd);
                for (size_t j = 0; j < n->carg_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->cargs[j]);
                    queue[i++] = n->cargs[j];
                }
            } break;
            case AST_CASE: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "case");
                fprintf(out, "v_%p -- v_%p\n", n, n->cexpr);
                fprintf(out, "v_%p -- v_%p [label=\"%s\"]\n", n, n->pred, "if");
                queue[i++] = n->cexpr;
                queue[i++] = n->pred;
            } break;
            case AST_CASES: {
                fprintf(out, "v_%p[label=\"%s\"]\n", n, "cases");
                for (size_t j = 0; j < n->stmnt_count; ++j) {
                    fprintf(out, "v_%p -- v_%p\n", n, n->stmnts[j]);
                    queue[i++] = n->stmnts[j];
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
    StringBuilder sb = {};

    switch (value->type) {
        case V_NIL:
            sb_append(&sb, "nil");
            break;
        case V_INT: {
            char* buf = malloc(256 * sizeof(char));
            sprintf(buf, "%lld", value->int_value);
            sb_append(&sb, buf);
        } break;
        case V_FLOAT: {
            char* buf = malloc(256 * sizeof(char));
            sprintf(buf, "%f", value->float_value);
            sb_append(&sb, buf);
        } break;
        case V_STRING: {
            sb_append(&sb, value->string_value);
        } break;
        case V_LIST: {
            sb_append(&sb, "[");
            for (size_t i = 0; i < value->list_size; ++i) {
                if (i > 0) {
                    sb_append(&sb, ", ");
                }
                sb_append(&sb, ast_value_to_str(value->list_value + i));
            }
            sb_append(&sb, "]");
        } break;
        case V_RANGE: {
            sb_append(&sb, ast_value_to_str(&value->range_value->start));
            sb_append(&sb, "..");
            sb_append(&sb, ast_value_to_str(&value->range_value->stop));
        } break;
        default:
            error("%s: unknown value type: %s\n", __PRETTY_FUNCTION__, value_type_to_str(value->type));
    }

    return sb_string(&sb);
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
        case TOK_PIPE:
            return "|";
        case TOK_AMP:
            return "&";
        default:
            error("unknown binop type: %s\n", tok_type_to_str(binop_type));
            exit(1);
    }
}

const char* unop_type_to_str(enum TokenType unop_type) {
    switch (unop_type) {
        case TOK_MINUS:
            return "-";
        case TOK_BANG:
            return "!";
        case TOK_HASH:
            return "#";
        default:
            error("unknown unop type: %s\n", tok_type_to_str(unop_type));
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

    node->stmnt_count = arr.size;
    node->stmnts = (struct AstNode**)arr.data;

    while (parser->tok->type == TOK_EOL) {
        parser->tok++;
    }
}

void parse_stmnt(struct Parser* parser, struct AstNode* node) {
    if (parser->tok->type == KW_for) {
        node->type = AST_FOR;
        parser->tok++;

        expect(TOK_IDENTIFIER);
        node->lvar = parser->tok->value;
        parser->tok++;

        expect(KW_in);
        parser->tok++;

        node->lexpr = node_new();
        parse_expr(parser, node->lexpr);

        if (parser->tok->type == TOK_EOL) {
            parser->tok++;
        }

        node->lbody = node_new();
        parse_stmnt(parser, node->lbody);
    } else if (parser->tok->type == TOK_CMD) {
        node->type = AST_CMD;
        node->cmd = parser->tok->value;
        node->carg_count = 0;
        parser->tok++;

        PtrArr arr = {};
        struct AstNode* tmp;
        while (parser->tok->type != TOK_EOL && parser->tok->type != TOK_EOF) {
            tmp = node_new();
            parse_expr(parser, tmp);
            ptrarr_append(&arr, tmp);
        }

        node->cargs = (struct AstNode**)arr.data;
        node->carg_count = arr.size;
    } else {
        parse_expr(parser, node);

        if (parser->tok->type == KW_if) {
            parser->tok++;
            struct AstNode* tmp = node_new();
            *tmp = *node;
            node->type = AST_CASE;
            node->cexpr = tmp;
            node->pred = node_new();
            parse_expr(parser, node->pred);
        }
    }
}

void parse_expr(struct Parser* parser, struct AstNode* node) {
    return parse_disj(parser, node);
}

void parse_disj(struct Parser* parser, struct AstNode* node) {
    parse_conj(parser, node);

    if (parser->tok->type == TOK_PIPE) {
        struct AstNode* rhs = node_new();
        *rhs = *node;
        node->type = AST_BINOP;
        node->lhs = rhs;
        node->rhs = node_new();
        node->binop_type = parser->tok->type;
        parser->tok++;
        parse_conj(parser, node->rhs);
    }
}

void parse_conj(struct Parser* parser, struct AstNode* node) {
    parse_comp(parser, node);

    if (parser->tok->type == TOK_AMP) {
        struct AstNode* rhs = node_new();
        *rhs = *node;
        node->type = AST_BINOP;
        node->lhs = rhs;
        node->rhs = node_new();
        node->binop_type = parser->tok->type;
        parser->tok++;
        parse_comp(parser, node->rhs);
    }
}

void parse_comp(struct Parser* parser, struct AstNode* node) {
    parse_range(parser, node);

    if (parser->tok->type == TOK_LT || parser->tok->type == TOK_GT || parser->tok->type == TOK_LEQ ||
        parser->tok->type == TOK_GEQ || parser->tok->type == TOK_EEQ || parser->tok->type == TOK_NEQ) {
        struct AstNode* rhs = node_new();
        *rhs = *node;
        node->type = AST_BINOP;
        node->lhs = rhs;
        node->rhs = node_new();
        node->binop_type = parser->tok->type;
        parser->tok++;
        parse_range(parser, node->rhs);
    }
}

void parse_range(struct Parser* parser, struct AstNode* node) {
    parse_sum(parser, node);

    if (parser->tok->type == TOK_DOTDOT) {
        struct AstNode* start = node_new();
        *start = *node;

        node->type = AST_RANGE;
        node->rstart = start;
        node->rcount = NULL;
        node->rstep = NULL;

        parser->tok++;
        node->rstop = node_new();
        parse_sum(parser, node->rstop);

        if (parser->tok->type == TOK_DOTDOT) {
            parser->tok++;
            struct AstNode* tmp = node_new();
            if (parser->tok->type == TOK_PLUS) {
                parser->tok++;
                node->rstep = tmp;
            } else {
                node->rcount = tmp;
            }
            parse_sum(parser, tmp);
        }
    }
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

        struct AstNode* rhs = node_new();
        parse_factor(parser, rhs);

        struct AstNode* lhs = node_new();
        *lhs = *node;

        node->type = AST_BINOP;
        node->binop_type = op;
        node->lhs = lhs;
        node->rhs = rhs;
    }
}

void parse_factor(struct Parser* parser, struct AstNode* node) {
    if (parser->tok->type == TOK_MINUS || parser->tok->type == TOK_HASH || parser->tok->type == TOK_BANG) {
        node->type = AST_UNOP;
        node->unop_type = parser->tok->type;
        parser->tok++;
        node->node = node_new();
        parse_factor(parser, node->node);
    } else {
        parse_atom(parser, node);

        if (parser->tok->type == TOK_POWER) {
            struct AstNode* lhs = node_new();
            *lhs = *node;
            node->type = AST_BINOP;
            node->binop_type = parser->tok->type;
            node->lhs = lhs;
            parser->tok++;
            node->rhs = node_new();
            parse_factor(parser, node->rhs);
        }
    }
}

void parse_atom_ident_tail(struct Parser* parser, struct AstNode* node) {
    switch (parser->tok->type) {
        case TOK_EQ: {
            Node_t* lhs = node_new();
            *lhs = *node;
            node->ident = lhs;
            node->type = AST_ASSIGNMENT;
            parser->tok++;
            node->rvalue = node_new();
            parse_expr(parser, node->rvalue);
        } break;
        case TOK_LPAREN: {
            parser->tok++;

            node->type = AST_FCALL;
            node->fname = node->name;

            if (parser->tok->type != TOK_RPAREN) {
                struct AstNode* tmp = node_new();
                tmp->item_count = 0;
                parse_items(parser, tmp);
                node->params = tmp->items;
                node->param_count = tmp->item_count;
            }

            expect(TOK_RPAREN);
            parser->tok++;

            if (parser->tok->type == TOK_EQ) {
                parser->tok++;

                node->type = AST_FDEF;
                for (size_t ip = 0; ip < node->param_count; ++ip) {
                    if (node->params[ip]->type != AST_IDENTIFIER) {
                        syntax_error("expected identifier but got %s\n", node_type_to_str(node->params[ip]->type));
                    }
                }
                node->fbody = node_new();
                parse_expr(parser, node->fbody);
            }
        } break;
        case TOK_LBRACKET: {
            parser->tok++;

            node->type = AST_IDX;
            node->lname = node->name;
            node->iexpr = node_new();

            parse_expr(parser, node->iexpr);
            expect(TOK_RBRACKET);
            parser->tok++;

            if (parser->tok->type == TOK_EQ) {
                parser->tok++;

                struct AstNode* tmp = node_new();
                *tmp = *node;
                node->type = AST_ASSIGNMENT;
                node->ident = tmp;
                node->rvalue = node_new();
                parse_expr(parser, node->rvalue);
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
    node->items = (struct AstNode**)arr.data;
    node->item_count = arr.size;
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

        if (arr.size == 1 && tmp->type == AST_CASE) {
            node->type = AST_CASES;
        }

        expect3(TOK_RBRACE, TOK_SEMICOLON, TOK_EOL);

        while (parser->tok->type == TOK_SEMICOLON || parser->tok->type == TOK_EOL) {
            parser->tok++;
        }
    }

    node->stmnts = (struct AstNode**)arr.data;
    node->stmnt_count = arr.size;

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
            if (parser->tok->type != TOK_RBRACKET) {
                parse_items(parser, node);
            } else {
                node->type = AST_ITEMS;
                node->item_count = 0;
            }
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
        case KW_Inf: {
            node->type = AST_LITERAL;
            node->value.type = V_INF;
            node->value.int_value = 1;
            parser->tok++;
        } break;
        case TOK_LBRACE: {
            parse_block(parser, node);
        } break;
        default:
            syntax_error("unexpected token: %s\n", tok_to_str(*parser->tok));
    };
}
