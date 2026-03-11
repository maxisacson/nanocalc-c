#ifndef NC_H
#define NC_H

#include <stddef.h>
#include <stdbool.h>

#define VALUE_TYPES \
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
    VALUE_TYPES
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
struct PlugSpec {
    const char* name;
    const char** func_names;
    size_t* func_nargs;
    Func_t* funcs;
    size_t nfuncs;
};
typedef struct PlugSpec PlugSpec_t;
typedef int (*PlugInitFunc_t)(PlugSpec_t*);

#endif
