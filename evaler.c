#include "evaler.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "utils.h"

typedef enum TokenType Binop_t;
typedef enum TokenType Unop_t;
typedef struct AstValue Value_t;
typedef struct AstNode Node_t;
typedef struct Context Context_t;
typedef struct RangeValue Range_t;
typedef struct EvalFunc EvalFunc_t;
typedef Value_t (*Cmd_t)(Context_t*, Node_t**, size_t);
typedef Value_t (*Func_t)(Value_t*, size_t);

const Value_t NIL = {.type = V_NIL};
const Value_t INF = {.type = V_INF, .int_value = 1};
const Value_t TRUE = {.type = V_INT, .int_value = 1};
const Value_t FALSE = {.type = V_INT, .int_value = 0};

Value_t range_next(Range_t*);

Value_t cmd_print(Context_t* context, Node_t** args, size_t arg_count) {
    for (size_t i = 0; i < arg_count; ++i) {
        Value_t value = eval(args[i], context);
        if (value.type == V_RANGE) {
            bool first = true;
            Range_t* range = value.range_value;
            printf("[");
            for (Value_t val = range_next(range); !range->done; val = range_next(range)) {
                if (!first) {
                    printf(", ");
                }
                const char* out = ast_value_to_str(&val);
                printf("%s", out);
                first = false;
            }
            printf("]");
        } else {
            if (i > 0) {
                printf(" ");
            }
            const char* out = ast_value_to_str(&value);
            printf("%s", out);
        }
    }
    printf("\n");

    return NIL;
};

typedef struct {
    const char* name;
    Cmd_t cmd;
} CmdItem_t;

const CmdItem_t commands[] = {
    {"print", cmd_print},
};

Cmd_t get_cmd(const char* name) {
    size_t count = sizeof(commands) / sizeof(commands[0]);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(name, commands[i].name) == 0) {
            return commands[i].cmd;
        }
    }

    return NULL;
};

double as_float(Value_t value) {
    switch (value.type) {
        case V_INT:
            return (double)value.int_value;
        case V_FLOAT:
            return value.float_value;
        default:
            eval_error("cannot cast to float: %s\n", value_type_to_str(value.type));
    }
}

Value_t make_int(long long x) {
    Value_t value = {.type = V_INT, .int_value = x};
    return value;
}

Value_t make_float(double x) {
    Value_t value = {.type = V_FLOAT, .float_value = x};
    return value;
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
    Context_t context = {
        .parent = parent,
        .map = new_map(),
        .read_only = false,
    };
    return context;
}

EvalFunc_t* evalfunc_new(Context_t* context, const char** params, size_t param_count, Node_t* body, Func_t func) {
    if (body != NULL && func != NULL) {
        error("cannot specify both body and func\n");
    }

    if (body == NULL && func == NULL) {
        error("must specify either body or func\n");
    }

    EvalFunc_t* ef = malloc(sizeof(EvalFunc_t));
    ef->context = context;
    ef->params = params;
    ef->param_count = param_count;
    ef->body = body;
    ef->func = func;

    return ef;
}

Value_t value_repeat(Value_t value, size_t count) {
    Value_t list = {.type = V_LIST, .list_size = count};
    list.list_value = malloc(count * sizeof(Value_t));

    for (size_t i = 0; i < count; ++i) {
        list.list_value[i] = value;
    }

    return list;
}

Value_t broadcast_func1(Value_t(*func)(Value_t), Value_t value) {
    Value_t result = NIL;

    if (value.type == V_LIST) {
        result.type = V_LIST;
        result.list_size = value.list_size;
        result.list_value = malloc(result.list_size * sizeof(Value_t));
        for (size_t i = 0; i < result.list_size; ++i) {
            result.list_value[i] = func(value.list_value[i]);
        }
    } else {
        result = func(value);
    }

    return result;
}

Value_t broadcast_func2(Value_t(*func)(Value_t, Value_t), Value_t lhs, Value_t rhs) {
    Value_t result = NIL;

    if (lhs.type == V_LIST && rhs.type == V_LIST) {
        if (lhs.list_size != rhs.list_size) {
            eval_error("expected lists to be of same length: %zu and %zu\n", lhs.list_size, rhs.list_size);
        }
        result.type = V_LIST;
        result.list_size = lhs.list_size;
        result.list_value = malloc(result.list_size * sizeof(Value_t));
        for (size_t i = 0; i < result.list_size; ++i) {
            result.list_value[i] = func(lhs.list_value[i], rhs.list_value[i]);
        }
    } else if (lhs.type == V_LIST) {
        rhs = value_repeat(rhs, lhs.list_size);
        result = broadcast_func2(func, lhs, rhs);
    } else if (rhs.type == V_LIST) {
        lhs = value_repeat(lhs, rhs.list_size);
        result = broadcast_func2(func, lhs, rhs);
    } else {
        result = func(lhs, rhs);
    }

    return result;
}

Value_t wrap_float_float1(double(*f)(double), Value_t value) {
    double x = as_float(value);
    double result = f(x);
    return make_float(result);
}

Value_t c_sin_impl(Value_t value) {
    return wrap_float_float1(sin, value);
}

Value_t c_cos_impl(Value_t value) {
    return wrap_float_float1(cos, value);
}

Value_t c_tan_impl(Value_t value) {
    return wrap_float_float1(tan, value);
}

Value_t c_asin_impl(Value_t value) {
    return wrap_float_float1(asin, value);
}

Value_t c_acos_impl(Value_t value) {
    return wrap_float_float1(acos, value);
}

Value_t c_atan_impl(Value_t value) {
    return wrap_float_float1(atan, value);
}

Value_t c_exp_impl(Value_t value) {
    return wrap_float_float1(exp, value);
}

Value_t c_log_impl(Value_t value) {
    return wrap_float_float1(log, value);
}

Value_t c_sqrt_impl(Value_t value) {
    return wrap_float_float1(sqrt, value);
}

Value_t c_sin(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_sin_impl, args[0]);
}

Value_t c_cos(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_cos_impl, args[0]);
}

Value_t c_tan(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_tan_impl, args[0]);
}

Value_t c_asin(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_asin_impl, args[0]);
}

Value_t c_acos(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_acos_impl, args[0]);
}

Value_t c_atan(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_atan_impl, args[0]);
}

Value_t c_exp(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_exp_impl, args[0]);
}

Value_t c_log(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_exp_impl, args[0]);
}

Value_t c_sqrt(Value_t* args, size_t nargs) {
    check_nargs(1);
    return broadcast_func1(c_sqrt_impl, args[0]);
}

Value_t make_callable(EvalFunc_t* f) {
    Value_t callable = {.type = V_CALLABLE, .data = f};
    return callable;
}

void setup_builtin_context(Context_t* context) {
    context->read_only = true;

    set_value(context, "sin", make_callable(evalfunc_new(context, NULL, 1, NULL, c_sin)));
    set_value(context, "cos", make_callable(evalfunc_new(context, NULL, 1, NULL, c_cos)));
    set_value(context, "tan", make_callable(evalfunc_new(context, NULL, 1, NULL, c_tan)));
    set_value(context, "asin", make_callable(evalfunc_new(context, NULL, 1, NULL, c_asin)));
    set_value(context, "acos", make_callable(evalfunc_new(context, NULL, 1, NULL, c_acos)));
    set_value(context, "atan", make_callable(evalfunc_new(context, NULL, 1, NULL, c_atan)));
    set_value(context, "exp", make_callable(evalfunc_new(context, NULL, 1, NULL, c_exp)));
    set_value(context, "log", make_callable(evalfunc_new(context, NULL, 1, NULL, c_log)));
    set_value(context, "sqrt", make_callable(evalfunc_new(context, NULL, 1, NULL, c_sqrt)));
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

bool is_negative(Value_t value) {
    switch (value.type) {
        case V_INT:
            return value.int_value < 0;
        case V_FLOAT:
            return value.float_value < 0;
        default:
            eval_error("incompatible type: %s\n", value_type_to_str(value.type));
    };
}

bool is_truthy(Value_t value) {
    switch (value.type) {
        case V_INT:
            return value.int_value != 0;
        case V_FLOAT:
            return value.float_value != 0;
        case V_STRING:
            return strlen(value.string_value) > 0;
        case V_LIST:
            return value.list_size > 0;
        default:
            return false;
    }
}

#define binop_impl(op)                                                                                          \
    if (lhs.type == V_INT && rhs.type == V_INT) {                                                                \
        result.type = V_INT;                                                                                     \
        result.int_value = lhs.int_value op rhs.int_value;                                                       \
    } else if (lhs.type == V_INT && rhs.type == V_FLOAT) {                                                       \
        result.type = V_FLOAT;                                                                                   \
        result.float_value = lhs.int_value op rhs.float_value;                                                   \
    } else if (lhs.type == V_FLOAT && rhs.type == V_INT) {                                                       \
        result.type = V_FLOAT;                                                                                   \
        result.float_value = lhs.float_value op rhs.int_value;                                                   \
    } else if (lhs.type == V_FLOAT && rhs.type == V_FLOAT) {                                                     \
        result.type = V_FLOAT;                                                                                   \
        result.float_value = lhs.float_value op rhs.float_value;                                                 \
    } else {                                                                                                     \
        eval_error("incompatible types: %s and %s\n", value_type_to_str(lhs.type), value_type_to_str(rhs.type)); \
    }

#define comp_impl(op)                                                                                           \
    if (lhs.type == V_INT && rhs.type == V_INT) {                                                                \
        result = lhs.int_value op rhs.int_value ? TRUE : FALSE;                                                  \
    } else if (lhs.type == V_INT && rhs.type == V_FLOAT) {                                                       \
        result = lhs.int_value op rhs.float_value ? TRUE : FALSE;                                                \
    } else if (lhs.type == V_FLOAT && rhs.type == V_INT) {                                                       \
        result = lhs.float_value op rhs.int_value ? TRUE : FALSE;                                                \
    } else if (lhs.type == V_FLOAT && rhs.type == V_FLOAT) {                                                     \
        result = lhs.float_value op rhs.float_value ? TRUE : FALSE;                                              \
    } else {                                                                                                     \
        eval_error("incompatible types: %s and %s\n", value_type_to_str(lhs.type), value_type_to_str(rhs.type)); \
    }

Value_t op_plus(Value_t lhs, Value_t rhs) {
    Value_t result;
    binop_impl(+);
    return result;
}

Value_t op_minus(Value_t lhs, Value_t rhs) {
    Value_t result;
    binop_impl(-);
    return result;
}

Value_t op_times(Value_t lhs, Value_t rhs) {
    Value_t result;
    binop_impl(*);
    return result;
}

Value_t op_divide(Value_t lhs, Value_t rhs) {
    Value_t result;
    binop_impl(/);
    return result;
}

Value_t op_mod(Value_t lhs, Value_t rhs) {
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
        fprintf(stderr, "eval_error: incompatible types: %d and %d\n", lhs.type, rhs.type);
        exit(1);
    }
    return result;
}

Value_t op_power(Value_t lhs, Value_t rhs) {
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
        fprintf(stderr, "eval_error: incompatible types: %d and %d\n", lhs.type, rhs.type);
        exit(1);
    }

    return result;
}

Value_t op_or(Value_t lhs, Value_t rhs) {
    if (is_truthy(lhs) || is_truthy(rhs)) {
        return TRUE;
    }

    return FALSE;
}

Value_t op_and(Value_t lhs, Value_t rhs) {
    if (is_truthy(lhs) && is_truthy(rhs)) {
        return TRUE;
    }

    return FALSE;
}

Value_t op_lt(Value_t lhs, Value_t rhs) {
    Value_t result;
    comp_impl(<);
    return result;
}

Value_t op_gt(Value_t lhs, Value_t rhs) {
    Value_t result;
    comp_impl(>);
    return result;
}

Value_t op_leq(Value_t lhs, Value_t rhs) {
    Value_t result;
    comp_impl(<=);
    return result;
}

Value_t op_geq(Value_t lhs, Value_t rhs) {
    Value_t result;
    comp_impl(>=);
    return result;
}

Value_t op_eeq(Value_t lhs, Value_t rhs) {
    Value_t result;
    comp_impl(==);
    return result;
}

Value_t op_neq(Value_t lhs, Value_t rhs) {
    Value_t result;
    comp_impl(!=);
    return result;
}

Value_t op_unary_minus(Value_t val) {
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

Value_t op_unary_not(Value_t val) {
    if (is_truthy(val)) {
        return FALSE;
    }

    return TRUE;
}

bool in_range(Value_t value, Value_t begin, Value_t end) {
    if (end.type == V_INF) {
        return true;
    }

    Value_t diff = op_minus(end, begin);
    if (is_negative(diff)) {
        Value_t tmp = begin;
        begin = end;
        end = tmp;
    }
    Value_t diff1 = op_minus(value, begin);
    Value_t diff2 = op_minus(end, value);

    if (is_negative(diff1) || is_negative(diff2)) {
        return false;
    }

    return true;
}

Range_t* range_new(Value_t start, Value_t stop, Value_t step, size_t length) {
    Range_t* range = malloc(sizeof(Range_t));
    range->start = start;
    range->stop = stop;
    range->step = step;
    range->value = NIL;
    range->count = 0;
    range->length = length;
    range->done = false;
    range->started = false;
    return range;
}

Value_t range_next(Range_t* range) {
    if (range->done) {
        eval_error("attempt to call next on an exhausted range\n");
    }

    if (range->length != UNDEF_SIZE) {
        if (range->count == range->length) {
            range->value = NIL;
            range->done = true;
        } else if (range->count + 1 == range->length) {
            range->value = range->stop;
        } else if (range->started) {
            range->value = op_plus(range->value, range->step);
        } else {
            range->value = range->start;
            range->started = true;
        }
        range->count++;
    } else {
        if (range->started) {
            range->value = op_plus(range->value, range->step);
        } else {
            range->value = range->start;
            range->started = true;
        }

        if (!in_range(range->value, range->start, range->stop)) {
            range->value = NIL;
            range->done = true;
        }
    }

    return range->value;
}

Value_t range_to_list(Range_t* range) {
    Value_t* values;
    size_t cap;
    size_t len = 0;

    if (range->length != UNDEF_SIZE) {
        cap = range->length;
    } else if (range->start.type == V_INF || range->stop.type == V_INF) {
        eval_error("cannot enumerate infinite range\n");
    } else {
        cap = 1;
    }
    values = malloc(cap * sizeof(Value_t));

    for (Value_t val = range_next(range); !range->done; val = range_next(range)) {
        if (len >= cap) {
            cap *= 2;
            values = realloc(values, cap * sizeof(Value_t));
        }

        values[len++] = val;
    }

    Value_t list = {
        .type = V_LIST,
        .list_size = len,
        .list_value = values,
    };

    return list;
}

Value_t eval_or(Context_t* context, Node_t* lhs, Node_t* rhs) {
    Value_t lval = eval(lhs, context);

    if (lval.type == V_LIST && lval.list_size > 0) {
        return broadcast_func2(op_or, lval, eval(rhs, context));
    }

    if (is_truthy(lval)) {
        return TRUE;
    }

    Value_t rval = eval(rhs, context);

    if (rval.type == V_LIST && rval.list_size > 0) {
        return broadcast_func2(op_or, lval, rval);
    }

    if (is_truthy(rval)) {
        return TRUE;
    }

    return FALSE;
}

Value_t eval_and(Context_t* context, Node_t* lhs, Node_t* rhs) {
    Value_t lval = eval(lhs, context);

    if (lval.type == V_LIST && lval.list_size > 0) {
        return broadcast_func2(op_and, lval, eval(rhs, context));
    }

    if (!is_truthy(lval)) {
        return FALSE;
    }

    Value_t rval = eval(rhs, context);

    if (rval.type == V_LIST && rval.list_size > 0) {
        return broadcast_func2(op_and, lval, rval);
    }

    if (!is_truthy(rval)) {
        return FALSE;
    }

    return TRUE;
}

Value_t eval_binop(Context_t* context, Binop_t op, Node_t* lhs, Node_t* rhs) {
    switch (op) {
        case TOK_PLUS:
            return broadcast_func2(op_plus, eval(lhs, context), eval(rhs, context));
        case TOK_MINUS:
            return broadcast_func2(op_minus, eval(lhs, context), eval(rhs, context));
        case TOK_STAR:
            return broadcast_func2(op_times, eval(lhs, context), eval(rhs, context));
        case TOK_FSLASH:
            return broadcast_func2(op_divide, eval(lhs, context), eval(rhs, context));
        case TOK_PERC:
            return broadcast_func2(op_mod, eval(lhs, context), eval(rhs, context));
        case TOK_POWER:
            return broadcast_func2(op_power, eval(lhs, context), eval(rhs, context));
        case TOK_LT:
            return broadcast_func2(op_lt, eval(lhs, context), eval(rhs, context));
        case TOK_GT:
            return broadcast_func2(op_gt, eval(lhs, context), eval(rhs, context));
        case TOK_LEQ:
            return broadcast_func2(op_leq, eval(lhs, context), eval(rhs, context));
        case TOK_GEQ:
            return broadcast_func2(op_geq, eval(lhs, context), eval(rhs, context));
        case TOK_EEQ:
            return broadcast_func2(op_eeq, eval(lhs, context), eval(rhs, context));
        case TOK_NEQ:
            return broadcast_func2(op_neq, eval(lhs, context), eval(rhs, context));
        case TOK_PIPE:
            return eval_or(context, lhs, rhs);
        case TOK_AMP:
            return eval_and(context, lhs, rhs);
        default:
            eval_error("unknown binop type: %s\n", tok_type_to_str(op));
    };
}

Value_t op_length(Value_t value) {
    Value_t result = {.type = V_INT};
    switch (value.type) {
        case V_LIST: {
            result.int_value = value.list_size;
        } break;
        case V_RANGE: {
            Value_t list = range_to_list(value.range_value);
            result.int_value = list.list_size;
        } break;
        case V_STRING: {
            result.int_value = (long long)strlen(value.string_value);
        } break;
        default:
            eval_error("value type has no defined length: %s\n", value_type_to_str(value.type));
    }

    return result;
}

Value_t eval_unop(Context_t* context, Unop_t op, Node_t* node) {
    switch (op) {
        case TOK_MINUS:
            return broadcast_func1(op_unary_minus, eval(node, context));
        case TOK_BANG:
            return broadcast_func1(op_unary_not, eval(node, context));
        case TOK_HASH:
            return op_length(eval(node, context));
        default:
            eval_error("unknown unop type: %s\n", tok_type_to_str(op));
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

    Value_t* args = malloc(param_count * sizeof(Value_t));
    for (size_t i = 0; i < param_count; ++i) {
        Value_t param = eval(params[i], context);
        args[i] = param;
    }

    Value_t result = NIL;
    if (f->body != NULL) {
        for (size_t i = 0; i < param_count; ++i) {
            set_value(&local, f->params[i], args[i]);
        }
        result = eval(f->body, &local);
    } else if (f->func != NULL) {
        result = f->func(args, param_count);
    }

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

    EvalFunc_t* ef = evalfunc_new(context, names, param_count, body, NULL);
    Value_t callable = {.type = V_CALLABLE, .data = ef};
    set_value(context, fname, callable);

    return NIL;
}

Value_t eval_for(Context_t* context, const char* name, Node_t* expr, Node_t* body) {
    Value_t values = eval(expr, context);

    Value_t value = NIL;

    if (values.type == V_LIST) {
        for (size_t i = 0; i < values.list_size; ++i) {
            set_value(context, name, values.list_value[i]);
            value = eval(body, context);
        }

    } else if (values.type == V_RANGE) {
        Range_t* range = values.range_value;
        for (Value_t val = range_next(range); !range->done; val = range_next(range)) {
            set_value(context, name, val);
            value = eval(body, context);
        }
    } else {
        set_value(context, name, values);
        value = eval(body, context);
    }

    return value;
}

Value_t make_float_range_count(double xstart, double xstop, size_t xcount) {
    Value_t value = {.type = V_RANGE};

    double xstep = (xstop - xstart) / (xcount - 1);
    value.range_value = range_new(make_float(xstart), make_float(xstop), make_float(xstep), xcount);

    return value;
}

Value_t make_int_range_count(long long xstart, long long xstop, size_t xcount) {
    Value_t value = {.type = V_RANGE};

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

    value.range_value = range_new(make_int(xstart), make_int(xstop), make_int(step), xcount);

    return value;
}

Value_t make_int_range_step(long long xstart, long long xstop, long long step) {
    Value_t value = {.type = V_RANGE};

    if (step < 0) {
        step = -step;
    }

    if (xstart > xstop) {
        step = -step;
    }

    value.range_value = range_new(make_int(xstart), make_int(xstop), make_int(step), UNDEF_SIZE);

    return value;
}

Value_t make_int_range_inf(long long xstart, long long step) {
    Value_t value = {.type = V_RANGE};

    if (step < 0) {
        step = -step;
    }

    value.range_value = range_new(make_int(xstart), INF, make_int(step), UNDEF_SIZE);

    return value;
}

Value_t make_float_range_step(double xstart, double xstop, double step) {
    Value_t value = {.type = V_RANGE};

    step = fabs(step);
    if (xstart > xstop) {
        step = -step;
    }

    value.range_value = range_new(make_float(xstart), make_float(xstop), make_float(step), UNDEF_SIZE);

    return value;
}

Value_t make_float_range_inf(double xstart, double step) {
    Value_t value = {.type = V_RANGE};

    step = fabs(step);

    value.range_value = range_new(make_float(xstart), INF, make_float(step), UNDEF_SIZE);

    return value;
}

Value_t eval_range(Context_t* context, Node_t* start, Node_t* stop, Node_t* count, Node_t* step) {
    Value_t value = NIL;

    Value_t vstart = eval(start, context);
    Value_t vstop = eval(stop, context);

    if (vstop.type == V_INF) {
        if (count) {
            Value_t vcount = eval(count, context);
            if (vcount.type != V_INT) {
                eval_error("expected count to be integer but got: %s\n", value_type_to_str(vcount.type));
            }

            if (vstart.type == V_INT) {
                long long xstop = vstart.int_value + vcount.int_value - 1;
                value = make_int_range_count(vstart.int_value, xstop, vcount.int_value);
            } else {
                double xstop = vstart.float_value + vcount.int_value - 1;
                value = make_float_range_count(as_float(vstart), xstop, vcount.int_value);
            }
        } else if (step) {
            Value_t vstep = eval(step, context);

            if (vstart.type == V_INT && vstep.type == V_INT) {
                value = make_int_range_inf(vstart.int_value, vstep.int_value);
            } else {
                value = make_float_range_inf(as_float(vstart), as_float(vstep));
            }
        } else {
            if (vstart.type == V_INT) {
                value = make_int_range_inf(vstart.int_value, 1);
            } else {
                value = make_float_range_inf(as_float(vstart), 1.0);
            }
        }

        return value;
    }

    if ((vstart.type != V_INT && vstart.type != V_FLOAT) || (vstop.type != V_INT && vstop.type != V_FLOAT)) {
        eval_error("incompatible types: %s and %s\n", value_type_to_str(vstart.type), value_type_to_str(vstop.type));
    }

    if (count) {
        Value_t vcount = eval(count, context);
        if (vcount.type != V_INT) {
            eval_error("expected count to be integer but got: %s\n", value_type_to_str(vcount.type));
        }

        if (vstart.type == V_INT && vstop.type == V_INT) {
            value = make_int_range_count(vstart.int_value, vstop.int_value, vcount.int_value);
        } else {
            value = make_float_range_count(as_float(vstart), as_float(vstop), vcount.int_value);
        }
    } else if (step) {
        Value_t vstep = eval(step, context);

        if (vstep.type != V_INT && vstep.type != V_FLOAT) {
            eval_error("expected step to be integer or float but got: %s\n", value_type_to_str(vstep.type));
        }

        if (vstart.type == V_INT && vstop.type == V_INT && vstep.type == V_INT) {
            value = make_int_range_step(vstart.int_value, vstop.int_value, vstep.int_value);
        } else {
            value = make_float_range_step(as_float(vstart), as_float(vstop), as_float(vstep));
        }
    } else {
        if (vstart.type == V_INT && vstop.type == V_INT) {
            value = make_int_range_step(vstart.int_value, vstop.int_value, 1);
        } else {
            value = make_float_range_count(as_float(vstart), as_float(vstop), 100);
        }
    }

    return value;
}

Value_t eval_cmd(Context_t* context, const char* name, Node_t** args, size_t arg_count) {
    Cmd_t cmd = get_cmd(name);
    Value_t value = cmd(context, args, arg_count);
    return value;
};

Value_t eval_case(Context_t* context, Node_t* expr, Node_t* pred) {
    Value_t value = NIL;

    Value_t p = eval(pred, context);
    if (is_truthy(p)) {
        value = eval(expr, context);
    }

    return value;
};

Value_t eval_cases(Context_t* context, Node_t** stmnts, size_t stmnt_count) {
    Value_t result = NIL;

    for (size_t i = 0; i < stmnt_count; ++i) {
        result = eval(stmnts[i], context);
        if (result.type != V_NIL) {
            return result;
        }
    }

    return result;
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
        case AST_CMD:
            return eval_cmd(context, node->cmd, node->cargs, node->carg_count);
        case AST_CASE:
            return eval_case(context, node->cexpr, node->pred);
        case AST_CASES:
            return eval_cases(context, node->stmnts, node->stmnt_count);
        default:
            eval_error("unknown AST node type: %s\n", node_type_to_str(node->type));
            exit(1);
    };
}
