#include <stdlib.h>
#include "token.h"

const char* tok_to_str(struct Token t) {
    const char* tt;

    switch (t.type) {
#define X(x)     \
    case x:      \
        tt = #x; \
        break;
        TOKEN_TYPES
#undef X
        default:
            fprintf(stderr, "token_error: unknown token type: %d\n", t.type);
            exit(1);
    }

    char* buf = malloc(256);
    if (t.value) {
        sprintf(buf, "%s(%s)", tt, t.value);
    } else {
        sprintf(buf, "%s", tt);
    }
    return buf;
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

    while ('0' <= **ptr && **ptr <= '9') {
        *in = **ptr;
        ++in;
        ++*ptr;
    }

    if (**ptr == '.') {
        *in = **ptr;
        ++in;
        ++*ptr;
    }

    while ('0' <= **ptr && **ptr <= '9') {
        *in = **ptr;
        ++in;
        ++*ptr;
    }

    if (**ptr == 'e' || **ptr == 'E') {
        *in = **ptr;
        ++in;
        ++*ptr;
        if (**ptr == '-') {
            *in = **ptr;
            ++in;
            ++*ptr;
        }
    }

    while ('0' <= **ptr && **ptr <= '9') {
        *in = **ptr;
        ++in;
        ++*ptr;
    }

    ta_append(arr, NUMBER, value);
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
                ta_append(&arr, EOL, 0);
                ++s;
                break;
            case '#':
                if (*peek == ' ') {
                    while ((*++s) != '\n') {
                    }
                    break;
                }
                ta_append(&arr, HASH, 0);
                ++s;
                break;
            case '<':
                if (*peek == '=') {
                    ta_append(&arr, LEQ, 0);
                    s+=2;
                } else {
                    ta_append(&arr, LT, 0);
                    ++s;
                }
                break;
            case '>':
                if (*peek == '=') {
                    ta_append(&arr, GEQ, 0);
                    s+=2;
                } else {
                    ta_append(&arr, GT, 0);
                    ++s;
                }
                break;
            case '=':
                if (*peek == '=') {
                    ta_append(&arr, EEQ, 0);
                    s+=2;
                } else {
                    ta_append(&arr, EQ, 0);
                    ++s;
                }
                break;
            case '!':
                if (*peek == '=') {
                    s += 2;
                    ta_append(&arr, NEQ, 0);
                    break;
                }
            case '-':
                ta_append(&arr, MINUS, 0);
                ++s;
                break;
            case '+':
                ta_append(&arr, PLUS, 0);
                ++s;
                break;
            case '*':
                ta_append(&arr, STAR, 0);
                ++s;
                break;
            case '/':
                ta_append(&arr, FSLASH, 0);
                ++s;
                break;
            case '^':
                ta_append(&arr, POWER, 0);
                ++s;
                break;
            case '%':
                ta_append(&arr, PERC, 0);
                ++s;
                break;
            case ',':
                ta_append(&arr, COMMA, 0);
                ++s;
                break;
            case '(':
                ta_append(&arr, LPAREN, 0);
                ++s;
                break;
            case ')':
                ta_append(&arr, RPAREN, 0);
                ++s;
                break;
            case ':':
                ta_append(&arr, COLON, 0);
                ++s;
                break;
            case ';':
                ta_append(&arr, SEMICOLON, 0);
                ++s;
                break;
            case '[':
                ta_append(&arr, LBRACKET, 0);
                ++s;
                break;
            case ']':
                ta_append(&arr, RBRACKET, 0);
                ++s;
                break;
            case '{':
                ta_append(&arr, LBRACE, 0);
                ++s;
                break;
            case '}':
                ta_append(&arr, RBRACE, 0);
                ++s;
                break;
            case '.':
                tok_number(&arr, &s);
                break;
            default:
                if ('0' <= *s && *s <= '9') {
                    tok_number(&arr, &s);
                } else {
                    fprintf(stderr, "token_error: unknown token: %c\n", *s);
                    exit(1);
                }
                break;
        };
    }

    ta_append(&arr, END, 0);
    *tokens = arr.data;

    return arr.size;
}
