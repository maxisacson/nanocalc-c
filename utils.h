#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdlib.h>

#define lineno() fprintf(stderr, "%s:%d -> %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)

#define error(...)                          \
    lineno();                               \
    fprintf(stderr, "error: " __VA_ARGS__); \
    exit(1)

#define syntax_error(...)                          \
    lineno();                                      \
    fprintf(stderr, "syntax_error: " __VA_ARGS__); \
    exit(1)

#define eval_error(...)                          \
    lineno();                                    \
    fprintf(stderr, "eval_error: " __VA_ARGS__); \
    exit(1)

#define incompatible_types(typea, typeb) \
    eval_error("incompatible types: %s and %s\n", value_type_to_str(typea), value_type_to_str(typeb));

#define unreachable_code()                                                                     \
    fprintf(stderr, "unreachable_code: %s: %s:%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    abort()

typedef struct {
    const char** data;
    size_t size;
    size_t capacity;
} StringBuilder;

void sb_append(StringBuilder* sb, const char* str);
void sb_append_v(StringBuilder* sb, size_t count, const char* strings[]);
const char* sb_string(StringBuilder* sb);
StringBuilder sb_join(const char* sep, const char** strs, size_t strc);

#define sb_append_n(s, n, ...)             \
    do {                                   \
        const char* tmp[] = {__VA_ARGS__}; \
        sb_append_v((s), (n), tmp);        \
    } while (0);

typedef struct {
    void** data;
    size_t size;
    size_t capacity;
} PtrArr;

void ptrarr_append(PtrArr* array, void* data);

#define UNDEF_SIZE (size_t)(-1)

#endif
