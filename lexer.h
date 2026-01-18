#ifndef LEXER_H
#define LEXER_H

#include "token.h"

const char* tok_to_str(struct Token t);
int tokenize(const char* string, struct Token* tokens[]);

#endif
