#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "token.h"
#include "lexer.h"

bool tok_is_keyword(const char* str) {
#define X(x)                    \
    if (strcmp(str, #x) == 0) { \
        return true;            \
    }
    KEYWORDS
#undef X

    return false;
}

bool tok_is_command(const char* str) {
#define X(x)                    \
    if (strcmp(str, #x) == 0) { \
        return true;            \
    }
    COMMANDS
#undef X

    return false;
}

enum TokenType tok_kw_to_tt(const char* str) {
#define X(x)                    \
    if (strcmp(str, #x) == 0) { \
        return KW_##x;          \
    }
    KEYWORDS
#undef X

    fprintf(stderr, "token_error: unknown keyword: %s\n", str);
    exit(1);
}

enum TokenType tok_cmd_to_tt(const char* str) {
#define X(x)                    \
    if (strcmp(str, #x) == 0) { \
        return CMD_##x;         \
    }
    COMMANDS
#undef X

    fprintf(stderr, "token_error: unknown keyword: %s\n", str);
    exit(1);
}

const char* tok_to_str(struct Token t) {
    const char* tt = tok_type_to_str(t.type);

    char* buf = malloc(256);
    if (t.value) {
        sprintf(buf, "%s(%s)", tt, t.value);
    } else {
        sprintf(buf, "%s", tt);
    }
    return buf;
}

const char* tok_type_to_str(enum TokenType tok_type) {
    const char* tt;

    switch (tok_type) {
#define X(x)     \
    case x:      \
        tt = #x; \
        break;
        TOKEN_TYPES
#undef X
        default:
            fprintf(stderr, "token_error: unknown token type: %d\n", tok_type);
            exit(1);
    }

    return tt;
}

// 'space': \s -> skip
// 'eol': \n
// 'comment': # .*$
// \0:
//   | [<>=!]=?
//   | [-+*/^%,\(\):;\[\]\{\}#]
// 'number':
//   | [0-9]+\.?[0-9]*([eE]-?)[0-9]*
//   | \.[0-9]*([eE]-?)[0-9]*
// 'identifier':
//   | [_a-zA-Z][_a-zA-Z0-9']*
// 'string': (?<=").*(?=")

struct TokenArray {
    size_t capacity;
    size_t size;
    struct Token* data;
};

int ta_append(struct TokenArray* arr, enum TokenType type, const char* value) {
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * sizeof(struct Token));
    }
    struct Token t = {.type = type, .value = value};
    arr->data[arr->size++] = t;
    return 0;
}

void tok_number(struct TokenArray* arr, const char** ptr) {
    char* value = malloc(512);
    char* in = value;

    enum TokenType tt = TOK_INTEGER;

    while ('0' <= **ptr && **ptr <= '9') {
        *in++ = *(*ptr)++;
    }

    if (**ptr == '.') {
        *in++ = *(*ptr)++;
        tt = TOK_FLOAT;
    }

    while ('0' <= **ptr && **ptr <= '9') {
        *in++ = *(*ptr)++;
    }

    if (**ptr == 'e' || **ptr == 'E') {
        *in++ = *(*ptr)++;
        if (**ptr == '-') {
            *in++ = *(*ptr)++;
        }
        tt = TOK_FLOAT;
    }

    while ('0' <= **ptr && **ptr <= '9') {
        *in++ = *(*ptr)++;
    }

    *in = 0;

    ta_append(arr, tt, value);
}

void tok_ident_or_keyword(struct TokenArray* arr, const char** ptr) {
    char* value = malloc(512);
    char* in = value;

    while (isalnum(**ptr) || **ptr == '_') {
        *in++ = *(*ptr)++;
    }
    *in = 0;

    if (tok_is_keyword(value)) {
        ta_append(arr, tok_kw_to_tt(value), 0);
    } else if (tok_is_command(value)) {
        ta_append(arr, tok_cmd_to_tt(value), 0);
    } else {
        ta_append(arr, TOK_IDENTIFIER, value);
    }
}

void tok_string(struct TokenArray* arr, const char** ptr) {
    char* value = malloc(512);
    char* in = value;

    ++*ptr;
    while (**ptr && **ptr != '"') {
        *in++ = *(*ptr)++;
    }
    ++*ptr;
    *in = 0;

    ta_append(arr, TOK_STRING, value);
}

int tokenize(const char* string, struct Token* tokens[]) {
    struct TokenArray arr = {.capacity = 16};
    arr.data = malloc(arr.capacity * sizeof(struct Token));

    const char* s = string;
    const char* peek;
    while (*s) {
        peek = s + 1;

        switch (*s) {
            case ' ':
                continue;
                ++s;
            case '\n':
                ta_append(&arr, TOK_EOL, 0);
                ++s;
                break;
            case '#':
                if (*peek == ' ') {
                    while ((*++s) != '\n') {
                    }
                    break;
                }
                ta_append(&arr, TOK_HASH, 0);
                ++s;
                break;
            case '<':
                if (*peek == '=') {
                    ta_append(&arr, TOK_LEQ, 0);
                    s += 2;
                } else {
                    ta_append(&arr, TOK_LT, 0);
                    ++s;
                }
                break;
            case '>':
                if (*peek == '=') {
                    ta_append(&arr, TOK_GEQ, 0);
                    s += 2;
                } else {
                    ta_append(&arr, TOK_GT, 0);
                    ++s;
                }
                break;
            case '=':
                if (*peek == '=') {
                    ta_append(&arr, TOK_EEQ, 0);
                    s += 2;
                } else {
                    ta_append(&arr, TOK_EQ, 0);
                    ++s;
                }
                break;
            case '!':
                if (*peek == '=') {
                    s += 2;
                    ta_append(&arr, TOK_NEQ, 0);
                    break;
                }
            case '-':
                ta_append(&arr, TOK_MINUS, 0);
                ++s;
                break;
            case '+':
                ta_append(&arr, TOK_PLUS, 0);
                ++s;
                break;
            case '*':
                ta_append(&arr, TOK_STAR, 0);
                ++s;
                break;
            case '/':
                ta_append(&arr, TOK_FSLASH, 0);
                ++s;
                break;
            case '^':
                ta_append(&arr, TOK_POWER, 0);
                ++s;
                break;
            case '%':
                ta_append(&arr, TOK_PERC, 0);
                ++s;
                break;
            case ',':
                ta_append(&arr, TOK_COMMA, 0);
                ++s;
                break;
            case '(':
                ta_append(&arr, TOK_LPAREN, 0);
                ++s;
                break;
            case ')':
                ta_append(&arr, TOK_RPAREN, 0);
                ++s;
                break;
            case ':':
                ta_append(&arr, TOK_COLON, 0);
                ++s;
                break;
            case ';':
                ta_append(&arr, TOK_SEMICOLON, 0);
                ++s;
                break;
            case '[':
                ta_append(&arr, TOK_LBRACKET, 0);
                ++s;
                break;
            case ']':
                ta_append(&arr, TOK_RBRACKET, 0);
                ++s;
                break;
            case '{':
                ta_append(&arr, TOK_LBRACE, 0);
                ++s;
                break;
            case '}':
                ta_append(&arr, TOK_RBRACE, 0);
                ++s;
                break;
            case '.':
                tok_number(&arr, &s);
                break;
            default:
                if ('0' <= *s && *s <= '9') {
                    tok_number(&arr, &s);
                } else if (isalpha(*s) || *s == '_') {
                    tok_ident_or_keyword(&arr, &s);
                } else if (*s == '"') {
                    tok_string(&arr, &s);
                } else {
                    fprintf(stderr, "token_error: unknown token: %c\n", *s);
                    exit(1);
                }
                break;
        };
    }

    ta_append(&arr, TOK_EOF, 0);
    *tokens = arr.data;

    return arr.size;
}
