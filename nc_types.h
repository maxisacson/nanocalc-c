#ifndef NC_TYPES_H
#define NC_TYPES_H

#include <stddef.h>
#include <stdbool.h>

#define NC_VALUE_TYPES_X \
    X(V_NIL)        \
    X(V_INT)        \
    X(V_FLOAT)      \
    X(V_STRING)     \
    X(V_LIST)       \
    X(V_RANGE)      \
    X(V_INF)        \
    X(V_CALLABLE)

enum ValueType {
#define X(x) x,
    NC_VALUE_TYPES_X
#undef X
};

struct RangeValue;

struct AstValue {
    enum ValueType type;
    union {
        // V_INT
        long long int int_value;

        // V_FLOAT
        double float_value;

        // V_STRING
        const char* string_value;

        // V_LIST
        struct {
            struct AstValue* list_value;
            size_t list_size;
        };

        // V_CALLABLE
        void* data;

        // V_RANGE
        struct RangeValue* range_value;
    };
};

struct RangeValue {
    struct AstValue start;
    struct AstValue stop;
    struct AstValue value;
    struct AstValue step;
    size_t count;
    size_t length;
    bool done;
    bool started;
};

typedef struct AstValue Value_t;
typedef Value_t (*Func_t)(size_t, Value_t*);

const char* value_type_to_str(enum ValueType value_type);
double as_float(Value_t value);
Value_t make_int(long long x);
Value_t make_float(double x);
bool is_negative(Value_t value);
bool is_truthy(Value_t value);

#define NC_INT(x) {.type = V_INT, .int_value = (x)}
#define NC_FLOAT(x) {.type = V_FLOAT, .float_value = (x)}
#define NC_AS_FLOAT(v) as_float(v)

#endif

// vim: ft=c
