#ifndef LEXER_H
#define LEXER_H

#include "token.h"

struct Lexer {
    int line;
    int col;
    size_t offset;
    const char* source;
};

struct Lexer lexer_new(const char* source);

const char* tok_to_str(struct Token t);
const char* tok_type_to_str(enum TokenType tok_type);
int tokenize(struct Lexer* lexer, struct Token* tokens[]);

#endif

// vim: ft=c
