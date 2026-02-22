#ifndef EVALER_H
#define EVALER_H

#include "parser.h"

struct Item {
    const char* key;
    struct AstValue value;
};

struct Map {
    size_t capacity;
    size_t size;
    struct Item* items;
};

struct Context {
    struct Context* parent;
    struct Map map;
    bool read_only;
};

struct EvalFunc {
    struct Context* context;
    const char** params;  // used for scripted functions
    size_t param_count;
    struct AstNode* body;                               // used for scripted functions
    struct AstValue (*func)(struct AstValue*, size_t);  // used for builtin functions
};

struct Map map_new();
struct Context context_new(struct Context* parent);
void setup_builtin_context(struct Context* context);

struct AstValue get_value(struct Context* context, const char* name);
void set_value(struct Context* context, const char* name, struct AstValue value);

struct AstValue eval(struct AstNode* node, struct Context* context);

#endif

// vim: ft=c
