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

double as_float(Value_t value) {
    switch (value.type) {
        case V_INT:
            return (double)value.int_value;
        case V_FLOAT:
            return value.float_value;
        default:
            eval_error("%s: cannot cast to float: %s\n", __PRETTY_FUNCTION__, value_type_to_str(value.type));
    }
}

size_t distance(long long a, long long b) {
    return a < b ? (size_t)(b - a) : (size_t)(a - b);
}

struct Map new_map() {
    struct Map map;
    map.capacity = 128;
    map.size = 0;
    map.items = malloc(map.capacity * sizeof(struct Item));
    return map;
}

Context_t context_new(Context_t* parent) {
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

Value_t eval_stmnts(Context_t* context, Node_t** stmnts, size_t stmnt_count) {
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

Value_t eval_fcall(Context_t* context, const char* fname, Node_t** params, size_t param_count) {
    Value_t callable = get_value(context, fname);
    struct EvalFunc* f = callable.data;

    struct Context local = context_new(f->context);

    for (size_t i = 0; i < param_count; ++i) {
        Value_t param = eval(params[i], context);
        set_value(&local, f->params[i], param);
    }

    Value_t result = eval(f->body, &local);

    return result;
}

Value_t eval_fdef(Context_t* context, const char* fname, Node_t** params, size_t param_count, Node_t* body) {
    const char** names = malloc(param_count * sizeof(const char*));

    for (size_t i = 0; i < param_count; ++i) {
        if (params[i]->type != AST_IDENTIFIER) {
            eval_error("expected identifier but got %s\n", node_type_to_str(params[i]->type));
        }
        names[i] = params[i]->name;
    }

    struct EvalFunc* ef = malloc(sizeof(struct EvalFunc));
    ef->context = context;
    ef->params = names;
    ef->param_count = param_count;
    ef->body = body;

    struct AstValue callable = {.data = ef};

    set_value(context, fname, callable);

    return NIL;
}

Value_t eval_for(Context_t* context, const char* name, Node_t* expr, Node_t* body) {
    Value_t values = eval(expr, context);

    if (values.type != V_LIST) {
        set_value(context, name, values);
        return eval(body, context);
    }

    Value_t value = NIL;
    for (size_t i = 0; i < values.list_size; ++i) {
        set_value(context, name, values.list_value[i]);
        value = eval(body, context);
    }

    return value;
}

Value_t make_float_range_count(double xstart, double xstop, size_t xcount) {
    Value_t value = {.type = V_LIST};

    double xstep = (xstop - xstart) / (xcount - 1);
    value.list_size = xcount;
    value.list_value = malloc(xcount * sizeof(Value_t));

    for (size_t i = 0; i < xcount; ++i) {
        Value_t val = {.type = V_FLOAT, .float_value = xstart + i * xstep};
        value.list_value[i] = val;
    }

    return value;
}

Value_t make_int_range_count(long long xstart, long long xstop, size_t xcount) {
    Value_t value = {.type = V_LIST};
    value.list_size = xcount;
    value.list_value = malloc(xcount * sizeof(Value_t));

    size_t divs = xcount - 1;
    size_t dist = distance(xstart, xstop);
    lldiv_t div = lldiv(dist, divs);

    if (div.rem != 0) {
        return make_float_range_count((double)xstart, (double)xstop, xcount);
    }

    long long step = 0;
    if (xstart < xstop) {
        step = div.quot;
    } else {
        step = -div.quot;
    }

    for (size_t i = 0; i < xcount; ++i) {
        Value_t val = {.type = V_INT, .int_value = xstart + i * step};
        value.list_value[i] = val;
    }

    return value;
}

Value_t make_int_range_step(long long xstart, long long xstop, long long step) {
    Value_t value = {.type = V_LIST};

    if (step < 0) {
        step = -step;
    }

    size_t dist = distance(xstart, xstop);
    size_t npoints = dist / step + 1;

    if (xstart > xstop) {
        step = -step;
    }

    value.list_size = npoints;
    value.list_value = malloc(npoints * sizeof(Value_t));

    for (size_t i = 0; i < npoints; ++i) {
        Value_t val = {.type = V_INT, .int_value = xstart + i * step};
        value.list_value[i] = val;
    }

    return value;
}

Value_t make_float_range_step(double xstart, double xstop, double step) {
    Value_t value = {.type = V_LIST};

    step = fabs(step);
    double dist = fabs(xstop - xstart);
    size_t npoints = (size_t)(dist / step) + 1;

    if (xstart > xstop) {
        step = -step;
    }

    value.list_size = npoints;
    value.list_value = malloc(npoints * sizeof(Value_t));

    for (size_t i = 0; i < npoints; ++i) {
        Value_t val = {.type = V_FLOAT, .float_value = xstart + i * step};
        value.list_value[i] = val;
    }

    return value;
}

Value_t eval_range(Context_t* context, Node_t* start, Node_t* stop, Node_t* count, Node_t* step) {
    Value_t vstart = eval(start, context);
    Value_t vstop = eval(stop, context);

    if ((vstart.type != V_INT && vstart.type != V_FLOAT) || (vstop.type != V_INT && vstop.type != V_FLOAT)) {
        eval_error("incompatible types: %s and %s\n", value_type_to_str(vstart.type), value_type_to_str(vstop.type));
    }

    Value_t value = NIL;
    value.type = V_LIST;

    if (count) {
        Value_t vcount = eval(count, context);

        if (vcount.type != V_INT) {
            eval_error("%s: expected count to be integer but got: %s\n", __PRETTY_FUNCTION__,
                       value_type_to_str(vcount.type));
        }

        if (vstart.type == V_INT && vstop.type == V_INT) {
            value = make_int_range_count(vstart.int_value, vstop.int_value, vcount.int_value);
        } else {
            value = make_float_range_count(as_float(vstart), as_float(vstop), vcount.int_value);
        }
    } else if (step) {
        Value_t vstep = eval(step, context);

        if (vstep.type != V_INT && vstep.type != V_FLOAT) {
            eval_error("%s: expected step to be integer or float but got: %s\n", __PRETTY_FUNCTION__,
                       value_type_to_str(vstep.type));
        }

        if (vstart.type == V_INT && vstop.type == V_INT) {
            if (vstep.type == V_INT) {
                value = make_int_range_step(vstart.int_value, vstop.int_value, vstep.int_value);
            } else {
                value = make_float_range_step(as_float(vstart), as_float(vstop), as_float(vstep));
            }
        } else {
            value = make_float_range_step(as_float(vstart), as_float(vstop), as_float(vstep));
        }
    } else {
        if (vstart.type == V_INT && vstop.type == V_INT) {
            size_t count = distance(vstart.int_value, vstop.int_value) + 1;
            value = make_int_range_count(vstart.int_value, vstop.int_value, count);
        } else {
            value = make_float_range_count(as_float(vstart), as_float(vstop), 100);
        }
    }

    return value;
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
            return eval_stmnts(context, node->stmnts, node->stmnt_count);
        case AST_ITEMS:
            return eval_items(context, node->items, node->item_count);
        case AST_FCALL:
            return eval_fcall(context, node->fname, node->params, node->param_count);
        case AST_FDEF:
            return eval_fdef(context, node->fname, node->params, node->param_count, node->fbody);
        case AST_BLOCK:
            return eval_stmnts(context, node->stmnts, node->stmnt_count);
        case AST_FOR:
            return eval_for(context, node->lvar, node->lexpr, node->lbody);
        case AST_RANGE:
            return eval_range(context, node->rstart, node->rstop, node->rcount, node->rstep);
        default:
            fprintf(stderr, "eval_error: %s: unknown AST node type: %s\n", __PRETTY_FUNCTION__,
                    node_type_to_str(node->type));
            exit(1);
    };
}
