#ifndef LEXER_H
#define LEXER_H

#include "token.h"

struct Lexer {
    int line;
    int col;
};

const char* tok_to_str(struct Token t);
const char* tok_type_to_str(enum TokenType tok_type);
int tokenize(struct Lexer* lexer, const char* string, struct Token* tokens[]);

#endif

// vim: ft=c
