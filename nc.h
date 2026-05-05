#ifndef NC_H
#define NC_H

#include "nc_types.h"
#include "nc_error.h"

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

#endif
