#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdlib.h>

#define error(...)                          \
    fprintf(stderr, "error: " __VA_ARGS__); \
    exit(1)

#define syntax_error(...)                          \
    fprintf(stderr, "syntax_error: " __VA_ARGS__); \
    exit(1)

#define eval_error(...)                          \
    fprintf(stderr, "eval_error: " __VA_ARGS__); \
    exit(1)

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} String;

void string_append(String* s, const char* str);
void string_append_v(String* s, size_t count, const char* strings[]);
void string_append_ch(String* s, char c);
void string_reserve(String* s, size_t size);
void string_set(String* s, const char* str);
String string_join(const char* sep, const char** strs, size_t strc);

#define string_append_n(s, n, ...)         \
    do {                                   \
        const char* tmp[] = {__VA_ARGS__}; \
        string_append_v((s), (n), tmp);    \
    } while (0);

typedef struct {
    void** data;
    size_t size;
    size_t capacity;
} PtrArr;

void ptrarr_append(PtrArr* array, void* data);

#endif
