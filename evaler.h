#ifndef EVALER_H
#define EVALER_H

#include "parser.h"

struct Context {

};

struct AstValue eval(struct AstNode* node, struct Context* context);

#endif
