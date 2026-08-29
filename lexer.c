#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "token.h"
#include "utils.h"
#include "lexer.h"

struct Lexer lexer_new(const char* source) {
    struct Lexer lexer = {
        .line = 1,
        .col = 0,
        .offset = 0,
        .source = source,
    };

    return lexer;
}

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

    StringBuilder sb = {};
    if (t.value) {
        const char* strs[] = {tt, "(", t.value, ")"};
        sb = sb_join("", 4, strs);
    } else {
        sb_append(&sb, tt);
    }
    return sb_string(&sb);
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

int ta_append(struct TokenArray* arr, enum TokenType type, const char* value, struct Lexer* lexer) {
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * sizeof(struct Token));
    }
    struct Token t = {.type = type,
                      .value = value,
                      .line = lexer->line,
                      .col = lexer->col,
                      .offset = lexer->offset,
                      .source = lexer->source};
    arr->data[arr->size++] = t;
    return 0;
}

void tok_number(struct Lexer* lexer, struct TokenArray* arr, const char** ptr) {
    const char* start = *ptr;

    enum TokenType tt = TOK_INTEGER;

    while ('0' <= **ptr && **ptr <= '9') {
        (*ptr)++;
    }

    if (**ptr == '.') {
        if (*(*ptr + 1) != '.') {
            (*ptr)++;
            tt = TOK_FLOAT;
        } else if (*(*ptr + 2) == '.') {
            (*ptr)++;
            tt = TOK_FLOAT;
        }
    }

    while ('0' <= **ptr && **ptr <= '9') {
        (*ptr)++;
    }

    if (**ptr == 'e' || **ptr == 'E') {
        (*ptr)++;
        if (**ptr == '-') {
            (*ptr)++;
        }
        tt = TOK_FLOAT;
    }

    while ('0' <= **ptr && **ptr <= '9') {
        (*ptr)++;
    }

    size_t len = *ptr - start;
    const char* value = strndup(start, len);

    ta_append(arr, tt, value, lexer);
    lexer->col += len;
}

void tok_ident_or_keyword(struct Lexer* lexer, struct TokenArray* arr, const char** ptr) {
    const char* start = *ptr;
    while (isalnum(**ptr) || **ptr == '_') {
        (*ptr)++;
    }
    size_t len = *ptr - start;
    const char* name = strndup(start, len);

    if (tok_is_keyword(name)) {
        ta_append(arr, tok_kw_to_tt(name), 0, lexer);
    } else if (tok_is_command(name)) {
        ta_append(arr, TOK_CMD, name, lexer);
    } else {
        ta_append(arr, TOK_IDENTIFIER, name, lexer);
    }

    lexer->col += len;
}

void tok_string(struct Lexer* lexer, struct TokenArray* arr, const char** ptr) {
    ++*ptr;
    const char* start = *ptr;
    while (**ptr && **ptr != '"') {
        (*ptr)++;
    }
    size_t len = *ptr - start;
    const char* value = strndup(start, len);
    ++*ptr;

    ta_append(arr, TOK_STRING, value, lexer);
    lexer->col += len + 1;
}

int tokenize(struct Lexer* lexer, struct Token* tokens[]) {
    struct TokenArray arr = {.capacity = 16};
    arr.data = malloc(arr.capacity * sizeof(struct Token));

    const char* s = lexer->source;
    const char* peek;
    while (*s) {
        peek = s + 1;
        lexer->offset = (size_t)(s - lexer->source);

        switch (*s) {
            case ' ':
                ++s;
                ++lexer->col;
                continue;
            case '\n':
                ta_append(&arr, TOK_EOL, 0, lexer);
                ++s;
                lexer->col = 0;
                ++lexer->line;
                break;
            case '#':
                if (*peek == ' ') {
                    while ((*++s) != '\n') {
                        ++lexer->col;
                    }
                    break;
                }
                ta_append(&arr, TOK_HASH, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '<':
                if (*peek == '=') {
                    ta_append(&arr, TOK_LEQ, 0, lexer);
                    s += 2;
                    lexer->col += 2;
                } else {
                    ta_append(&arr, TOK_LT, 0, lexer);
                    ++s;
                    ++lexer->col;
                }
                break;
            case '>':
                if (*peek == '=') {
                    ta_append(&arr, TOK_GEQ, 0, lexer);
                    s += 2;
                    lexer->col += 2;
                } else {
                    ta_append(&arr, TOK_GT, 0, lexer);
                    ++s;
                    ++lexer->col;
                }
                break;
            case '=':
                if (*peek == '=') {
                    ta_append(&arr, TOK_EEQ, 0, lexer);
                    s += 2;
                    lexer->col += 2;
                } else {
                    ta_append(&arr, TOK_EQ, 0, lexer);
                    ++s;
                    ++lexer->col;
                }
                break;
            case '!':
                if (*peek == '=') {
                    ta_append(&arr, TOK_NEQ, 0, lexer);
                    s += 2;
                    lexer->col += 2;
                } else {
                    ta_append(&arr, TOK_BANG, 0, lexer);
                    ++s;
                    ++lexer->col;
                }
                break;
            case '&':
                ta_append(&arr, TOK_AMP, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '|':
                ta_append(&arr, TOK_PIPE, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '-':
                ta_append(&arr, TOK_MINUS, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '+':
                ta_append(&arr, TOK_PLUS, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '*':
                ta_append(&arr, TOK_STAR, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '/':
                ta_append(&arr, TOK_FSLASH, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '^':
                ta_append(&arr, TOK_POWER, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '%':
                ta_append(&arr, TOK_PERC, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case ',':
                ta_append(&arr, TOK_COMMA, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '(':
                ta_append(&arr, TOK_LPAREN, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case ')':
                ta_append(&arr, TOK_RPAREN, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case ':':
                ta_append(&arr, TOK_COLON, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case ';':
                ta_append(&arr, TOK_SEMICOLON, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '[':
                ta_append(&arr, TOK_LBRACKET, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case ']':
                ta_append(&arr, TOK_RBRACKET, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '{':
                ta_append(&arr, TOK_LBRACE, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '}':
                ta_append(&arr, TOK_RBRACE, 0, lexer);
                ++s;
                ++lexer->col;
                break;
            case '.':
                if (*peek == '.') {
                    ta_append(&arr, TOK_DOTDOT, 0, lexer);
                    s += 2;
                    lexer->col += 2;
                } else {
                    tok_number(lexer, &arr, &s);
                }
                break;
            default:
                if ('0' <= *s && *s <= '9') {
                    tok_number(lexer, &arr, &s);
                } else if (isalpha(*s) || *s == '_') {
                    tok_ident_or_keyword(lexer, &arr, &s);
                } else if (*s == '"') {
                    tok_string(lexer, &arr, &s);
                } else {
                    fprintf(stderr, "token_error(%d:%d): unknown token: %c\n", lexer->line, lexer->col, *s);
                    exit(1);
                }
                break;
        };
    }

    ta_append(&arr, TOK_EOF, 0, lexer);
    *tokens = arr.data;

    return arr.size;
}
