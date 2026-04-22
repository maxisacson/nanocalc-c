#include <stdio.h>
#include <stdlib.h>

#define NC_IMPL
#include "nc.h"

static FuncSpec_t functions[] = {
    {.name = "hyp2", .nargs = 2},
    {.name = "hyp", .nargs = 2},
};

static PlugSpec_t plugin_spec = {
    .name = "math",
    .nfuncs = 2,
    .funcs = functions,
};

PlugSpec_t* init() {
    return &plugin_spec;
};

Value_t hyp2(size_t /* nargs */, Value_t* args) {
    Value_t x = args[0];
    Value_t y = args[1];

    double xf = NC_AS_FLOAT(x);
    double yf = NC_AS_FLOAT(y);

    Value_t result = NC_FLOAT(xf * xf + yf * yf);

    return result;
}

Value_t hyp(size_t /* nargs */, Value_t* args) {
    Value_t x = args[0];
    Value_t y = args[1];

    double xf = NC_AS_FLOAT(x);
    double yf = NC_AS_FLOAT(y);

    Value_t result = NC_FLOAT(sqrt(xf * xf + yf * yf));

    return result;
}
