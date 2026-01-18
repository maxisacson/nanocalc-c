#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

#define TOKEN_TYPES \
    X(WS)           \
    X(EOL)          \
    X(COMMENT)      \
    X(LEQ)          \
    X(GEQ)          \
    X(EEQ)          \
    X(NEQ)          \
    X(PLUS)         \
    X(MINUS)        \
    X(STAR)         \
    X(FSLASH)       \
    X(POWER)        \
    X(PERC)         \
    X(COMMA)        \
    X(LPAREN)       \
    X(RPAREN)       \
    X(COLON)        \
    X(SEMICOLON)    \
    X(LBRACKET)     \
    X(RBRACKET)     \
    X(LBRACE)       \
    X(RBRACE)       \
    X(LT)           \
    X(GT)           \
    X(EQ)           \
    X(HASH)         \
    X(NUMBER)       \
    X(STRING)       \
    X(IDENTIFIER)   \
    X(END)

#define X(x) x,
enum TokenType { TOKEN_TYPES };
#undef X

struct Token {
    enum TokenType type;
    const char* value;
};

#endif
