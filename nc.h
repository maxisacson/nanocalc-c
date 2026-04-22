#ifndef NC_H
#define NC_H

#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include "nc_error.h"

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

struct FuncSpec {
    const char* name;
    size_t nargs;
};
typedef struct FuncSpec FuncSpec_t;

struct PlugSpec {
    const char* name;
    size_t nfuncs;
    FuncSpec_t* funcs;
};
typedef struct PlugSpec PlugSpec_t;
typedef PlugSpec_t* (*PlugInitFunc_t)();

#define NC_INT(x) {.type = V_INT, .int_value = (x)}
#define NC_FLOAT(x) {.type = V_FLOAT, .float_value = (x)}
#define NC_AS_FLOAT(v) nc_as_float(v)

const char* nc_value_type_to_str(enum ValueType value_type);
double nc_as_float(Value_t value);

#ifdef NC_IMPL

inline const char* nc_value_type_to_str(enum ValueType value_type) {
    switch (value_type) {
#define X(x) \
    case x:  \
        return #x;
        NC_VALUE_TYPES_X
#undef X
        default:
            error("unknown value type: %d\n", value_type);
    }
}

inline double nc_as_float(Value_t value) {
    switch (value.type) {
        case V_INT:
            return (double)value.int_value;
            break;
        case V_FLOAT:
            return value.float_value;
            break;
        default:
            error("cannot cast to float: %s\n", nc_value_type_to_str(value.type));
            break;
    }
}

#endif

#endif
