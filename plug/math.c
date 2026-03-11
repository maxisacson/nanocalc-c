#include <stdio.h>
#include <stdlib.h>
#include "nc.h"

double as_float(Value_t value) {
    switch (value.type) {
        case V_INT:
            return (double)value.int_value;
        case V_FLOAT:
            return value.float_value;
        default:
            fprintf(stderr, "cannot cast value\n");
            exit(1);
    }
}

int init(struct PlugSpec* spec) {
    spec->name = "math";
    spec->nfuncs = 1;
    spec->func_names = (const char*[]){"hyp2"};
    spec->func_nargs = malloc(sizeof(size_t));
    spec->func_nargs[0] = 2;
    return 0;
};

Value_t hyp2(Value_t* args, size_t nargs) {
    Value_t x = args[0];
    Value_t y = args[1];

    double xf = as_float(x);
    double yf = as_float(y);

    Value_t result = {.type = V_FLOAT, .float_value = xf * xf + yf * yf};

    return result;
}
