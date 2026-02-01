#include "evaler.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

typedef enum TokenType Binop_t;
typedef enum TokenType Unop_t;
typedef struct AstValue Value_t;
typedef struct AstNode Node_t;
typedef struct Context Context_t;

const Value_t NIL = {.type = V_NIL};

struct Map new_map() {
    struct Map map;
    map.capacity = 128;
    map.size = 0;
    map.items = malloc(map.capacity * sizeof(struct Item));
    return map;
}

Context_t new_context(Context_t* parent) {
    Context_t context;
    context.parent = parent;
    context.map = new_map();
    return context;
}

Value_t get_value(Context_t* context, const char* name) {
    for (size_t i = 0; i < context->map.size; ++i) {
        if (strcmp(name, context->map.items[i].key) == 0) {
            return context->map.items[i].value;
        }
    }

    if (context->parent) {
        return get_value(context->parent, name);
    }

    return NIL;
}

void set_value(Context_t* context, const char* name, struct AstValue value) {
    for (size_t i = 0; i < context->map.size; ++i) {
        if (strcmp(name, context->map.items[i].key) == 0) {
            context->map.items[i].value = value;
            return;
        }
    }

    if (context->map.size >= context->map.capacity) {
        context->map.capacity *= 2;
        context->map.items = realloc(context->map.items, context->map.capacity * sizeof(struct Item));
    }

    struct Item item = {.key = name, .value = value};
    context->map.items[context->map.size++] = item;
}

#define apply_binop(op)                                                                                              \
    if (lhs.type == V_INT && rhs.type == V_INT) {                                                                    \
        result.type = V_INT;                                                                                         \
        result.int_value = lhs.int_value op rhs.int_value;                                                           \
    } else if (lhs.type == V_INT && rhs.type == V_FLOAT) {                                                           \
        result.type = V_FLOAT;                                                                                       \
        result.float_value = lhs.int_value op rhs.float_value;                                                       \
    } else if (lhs.type == V_FLOAT && rhs.type == V_INT) {                                                           \
        result.type = V_FLOAT;                                                                                       \
        result.float_value = lhs.float_value op rhs.int_value;                                                       \
    } else if (lhs.type == V_FLOAT && rhs.type == V_FLOAT) {                                                         \
        result.type = V_FLOAT;                                                                                       \
        result.float_value = lhs.float_value op rhs.float_value;                                                     \
    } else {                                                                                                         \
        fprintf(stderr, "eval_error: %s: incompatible types: %d and %d\n", __PRETTY_FUNCTION__, lhs.type, rhs.type); \
        exit(1);                                                                                                     \
    }

Value_t eval_plus(Value_t lhs, Value_t rhs) {
    Value_t result;
    apply_binop(+);
    return result;
}

Value_t eval_minus(Value_t lhs, Value_t rhs) {
    Value_t result;
    apply_binop(-);
    return result;
}

Value_t eval_times(Value_t lhs, Value_t rhs) {
    Value_t result;
    apply_binop(*);
    return result;
}

Value_t eval_divide(Value_t lhs, Value_t rhs) {
    Value_t result;
    apply_binop(/);
    return result;
}

Value_t eval_mod(Value_t lhs, Value_t rhs) {
    Value_t result;
    if (lhs.type == V_INT && rhs.type == V_INT) {
        result.type = V_INT;
        result.int_value = lhs.int_value % rhs.int_value;
    } else if (lhs.type == V_INT && rhs.type == V_FLOAT) {
        result.type = V_FLOAT;
        result.float_value = fmod((double)lhs.int_value, rhs.float_value);
    } else if (lhs.type == V_FLOAT && rhs.type == V_INT) {
        result.type = V_FLOAT;
        result.float_value = fmod(lhs.float_value, (double)rhs.int_value);
    } else if (lhs.type == V_FLOAT && rhs.type == V_FLOAT) {
        result.type = V_FLOAT;
        result.float_value = fmod(lhs.float_value, rhs.float_value);
    } else {
        fprintf(stderr, "eval_error: %s: incompatible types: %d and %d\n", __PRETTY_FUNCTION__, lhs.type, rhs.type);
        exit(1);
    }
    return result;
}

Value_t eval_power(Value_t lhs, Value_t rhs) {
    Value_t result;

    if (lhs.type == V_INT && rhs.type == V_INT) {
        result.type = V_INT;
        result.int_value = pow((double)lhs.int_value, (double)rhs.int_value);
    } else if (lhs.type == V_INT && rhs.type == V_FLOAT) {
        result.type = V_FLOAT;
        result.float_value = pow((double)lhs.int_value, rhs.float_value);
    } else if (lhs.type == V_FLOAT && rhs.type == V_INT) {
        result.type = V_FLOAT;
        result.float_value = pow(lhs.float_value, (double)rhs.int_value);
    } else if (lhs.type == V_FLOAT && rhs.type == V_FLOAT) {
        result.type = V_FLOAT;
        result.float_value = pow(lhs.float_value, rhs.float_value);
    } else {
        fprintf(stderr, "eval_error: %s: incompatible types: %d and %d\n", __PRETTY_FUNCTION__, lhs.type, rhs.type);
        exit(1);
    }

    return result;
}

Value_t eval_unary_minus(Value_t val) {
    Value_t result;
    if (val.type == V_INT) {
        result.type = V_INT;
        result.int_value = -val.int_value;
    } else if (val.type == V_FLOAT) {
        result.type = V_FLOAT;
        result.float_value = -val.float_value;
    }
    return result;
}

Value_t eval_binop(Context_t* context, Binop_t op, Node_t* lhs, Node_t* rhs) {
    switch (op) {
        case TOK_PLUS:
            return eval_plus(eval(lhs, context), eval(rhs, context));
        case TOK_MINUS:
            return eval_minus(eval(lhs, context), eval(rhs, context));
        case TOK_STAR:
            return eval_times(eval(lhs, context), eval(rhs, context));
        case TOK_FSLASH:
            return eval_divide(eval(lhs, context), eval(rhs, context));
        case TOK_PERC:
            return eval_mod(eval(lhs, context), eval(rhs, context));
        case TOK_POWER:
            return eval_power(eval(lhs, context), eval(rhs, context));
        default:
            eval_error("unknown binop type: %d\n", op);
    };
}

Value_t eval_unop(Context_t* context, Unop_t op, Node_t* node) {
    switch (op) {
        case TOK_MINUS:
            return eval_unary_minus(eval(node, context));
        default:
            eval_error("unknown unop type: %d\n", op);
    }
}

Value_t eval_assignment(Context_t* context, Node_t* ident, Value_t value) {
    if (ident->type == AST_IDENTIFIER) {
        set_value(context, ident->name, value);
    } else if (ident->type == AST_IDX) {
        Value_t list = get_value(context, ident->lname);
        if (list.type == V_NIL) {
            eval_error("did not find name in current context: %s\n", ident->lname);
        }
        Value_t idx = eval(ident->iexpr, context);
        if (idx.type != V_INT) {
            eval_error("cannot index using value type: %s\n", ident->lname);
        }
        list.list_value[idx.int_value] = value;
    } else {
        eval_error("unexpected lvalue type: %s\n", node_type_to_str(ident->type));
    }
    return value;
}

Value_t eval_identifier(Context_t* context, const char* name) {
    return get_value(context, name);
}

Value_t eval_program(Context_t* context, Node_t** stmnts, size_t stmnt_count) {
    Value_t result = NIL;

    for (size_t i = 0; i < stmnt_count; ++i) {
        result = eval(stmnts[i], context);
    }

    return result;
}

Value_t eval_items(Context_t* context, Node_t** items, size_t item_count) {
    Value_t result = NIL;

    result.type = V_LIST;
    result.list_size = item_count;
    result.list_value = malloc(item_count * sizeof(Value_t));

    for (size_t i = 0; i < item_count; ++i) {
        result.list_value[i] = eval(items[i], context);
    }

    return result;
}

struct EvalFunc {
    Context_t* context;
    const char** params;
    size_t param_count;
    Node_t* body;
};

Value_t eval_fcall(Context_t* context, const char* fname, Node_t* params) {
    Value_t callable = get_value(context, fname);
    struct EvalFunc* f = callable.data;

    struct Context local = new_context(f->context);

    Value_t plist = eval(params, context);

    for (size_t ip = 0; ip < params->item_count; ++ip) {
        set_value(&local, f->params[ip], plist.list_value[ip]);
    }

    Value_t result = eval(f->body, &local);

    return result;
}

Value_t eval_fdef(Context_t* context, const char* fname, Node_t* params, Node_t* body) {
    const char** pnames = malloc(params->item_count * sizeof(const char*));

    for (size_t ip = 0; ip < params->item_count; ++ip) {
        if (params->items[ip]->type != AST_IDENTIFIER) {
            eval_error("expected identifier but got %s\n", node_type_to_str(params->items[ip]->type));
        }
        pnames[ip] = params->items[ip]->name;
    }

    struct EvalFunc* ef = malloc(sizeof(struct EvalFunc));
    ef->context = context;
    ef->params = pnames;
    ef->param_count = params->item_count;
    ef->body = body;

    struct AstValue callable = {.data = ef};

    set_value(context, fname, callable);

    return NIL;
}

struct AstValue eval(struct AstNode* node, struct Context* context) {
    switch (node->type) {
        case AST_LITERAL:
            return node->value;
        case AST_BINOP:
            return eval_binop(context, node->binop_type, node->lhs, node->rhs);
        case AST_UNOP:
            return eval_unop(context, node->unop_type, node->node);
        case AST_IDENTIFIER:
            return eval_identifier(context, node->name);
        case AST_ASSIGNMENT:
            return eval_assignment(context, node->ident, eval(node->rvalue, context));
        case AST_PROGRAM:
            return eval_program(context, node->stmnts, node->stmnt_count);
        case AST_ITEMS:
            return eval_items(context, node->items, node->item_count);
        case AST_FCALL:
            return eval_fcall(context, node->fname, node->params);
        case AST_FDEF:
            return eval_fdef(context, node->fname, node->params, node->fbody);
        default:
            fprintf(stderr, "eval_error: %s: unknown AST node type: %s\n", __PRETTY_FUNCTION__,
                    node_type_to_str(node->type));
            exit(1);
    };
}
