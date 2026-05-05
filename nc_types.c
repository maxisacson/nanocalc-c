#include "nc_types.h"
#include "nc_error.h"

#include <string.h>

const char* value_type_to_str(enum ValueType value_type) {
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

double as_float(Value_t value) {
    switch (value.type) {
        case V_INT:
            return (double)value.int_value;
            break;
        case V_FLOAT:
            return value.float_value;
            break;
        default:
            eval_error("cannot cast to float: %s\n", value_type_to_str(value.type));
            break;
    }
}

Value_t make_int(long long x) {
    Value_t value = NC_INT(x);
    return value;
}

Value_t make_float(double x) {
    Value_t value = NC_FLOAT(x);
    return value;
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

