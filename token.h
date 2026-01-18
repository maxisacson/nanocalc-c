#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

#define KEYWORDS \
    X(if)        \
    X(and)       \
    X(or)        \
    X(not)       \
    X(for)       \
    X(in)        \
    X(Inf)

#define COMMANDS \
    X(print)     \
    X(write)     \
    X(table)     \
    X(sum)       \
    X(prod)      \
    X(dump)

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
    X(KW_if)        \
    X(KW_and)       \
    X(KW_or)        \
    X(KW_not)       \
    X(KW_for)       \
    X(KW_in)        \
    X(KW_Inf)       \
    X(CMD_print)    \
    X(CMD_write)    \
    X(CMD_table)    \
    X(CMD_sum)      \
    X(CMD_prod)     \
    X(CMD_dump)     \
    X(END)

#define X(x) x,
enum TokenType { TOKEN_TYPES };
#undef X

struct Token {
    enum TokenType type;
    const char* value;
};

#endif
