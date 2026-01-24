#ifndef LEXER_H
#define LEXER_H

#include "token.h"

const char* tok_to_str(struct Token t);
const char* tok_type_to_str(enum TokenType tok_type);
int tokenize(const char* string, struct Token* tokens[]);

#endif
