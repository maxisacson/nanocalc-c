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

#define TOKEN_TYPES   \
    X(TOK_WS)         \
    X(TOK_EOL)        \
    X(TOK_COMMENT)    \
    X(TOK_LEQ)        \
    X(TOK_GEQ)        \
    X(TOK_EEQ)        \
    X(TOK_NEQ)        \
    X(TOK_PLUS)       \
    X(TOK_MINUS)      \
    X(TOK_STAR)       \
    X(TOK_FSLASH)     \
    X(TOK_POWER)      \
    X(TOK_PERC)       \
    X(TOK_COMMA)      \
    X(TOK_LPAREN)     \
    X(TOK_RPAREN)     \
    X(TOK_COLON)      \
    X(TOK_SEMICOLON)  \
    X(TOK_LBRACKET)   \
    X(TOK_RBRACKET)   \
    X(TOK_LBRACE)     \
    X(TOK_RBRACE)     \
    X(TOK_LT)         \
    X(TOK_GT)         \
    X(TOK_EQ)         \
    X(TOK_HASH)       \
    X(TOK_BANG)       \
    X(TOK_INTEGER)    \
    X(TOK_FLOAT)      \
    X(TOK_STRING)     \
    X(TOK_IDENTIFIER) \
    X(TOK_EOF)        \
    X(KW_if)          \
    X(KW_and)         \
    X(KW_or)          \
    X(KW_not)         \
    X(KW_for)         \
    X(KW_in)          \
    X(KW_Inf)         \
    X(CMD_print)      \
    X(CMD_write)      \
    X(CMD_table)      \
    X(CMD_sum)        \
    X(CMD_prod)       \
    X(CMD_dump)

#define X(x) x,
enum TokenType { TOKEN_TYPES };
#undef X

struct Token {
    enum TokenType type;
    const char* value;
};

#endif
