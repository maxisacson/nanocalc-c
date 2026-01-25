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
};

struct Map new_map();
struct Context new_context(struct Context* parent);

struct AstValue get_value(struct Context* context, const char* name);
void set_value(struct Context* context, const char* name, struct AstValue value);

struct AstValue eval(struct AstNode* node, struct Context* context);

#endif
