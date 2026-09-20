#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdlib.h>
#include "nc_error.h"

#define check_nargs(expect)                                                 \
    if (nargs != (expect)) {                                                \
        eval_error("expected %d argument bot got: %zu\n", (expect), nargs); \
    }

typedef struct {
    const char** data;
    size_t size;
    size_t capacity;
} StringBuilder;

void sb_append(StringBuilder* sb, const char* str);
void sb_append_v(StringBuilder* sb, size_t count, const char* strings[]);
const char* sb_string(StringBuilder* sb);
StringBuilder sb_join(const char* sep, size_t strc, const char** strs);

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

#define make_array_def(type, struct_name, prefix)   \
    struct struct_name {                            \
        type* data;                                 \
        size_t cap;                                 \
        size_t size;                                \
    };                                              \
    typedef struct struct_name struct_name##_t;     \
    struct struct_name prefix##_create();           \
    void prefix##_destroy(struct struct_name* arr); \
    void prefix##_append(struct struct_name* arr, type x);

#define make_array_impl(type, struct_name, prefix)                          \
    struct struct_name prefix##_create() {                                  \
        struct struct_name arr = {.cap = 32, .size = 0};                    \
        arr.data = (type*)malloc(arr.cap * sizeof(type));                   \
        return arr;                                                         \
    }                                                                       \
                                                                            \
    void prefix##_destroy(struct struct_name* arr) {                        \
        arr->cap = 0;                                                       \
        arr->size = 0;                                                      \
        free(arr->data);                                                    \
        arr->data = NULL;                                                   \
    }                                                                       \
                                                                            \
    void prefix##_append(struct struct_name* arr, type x) {                 \
        if (arr->size >= arr->cap) {                                        \
            arr->cap = 2 * arr->cap;                                        \
            arr->data = (type*)realloc(arr->data, arr->cap * sizeof(type)); \
        }                                                                   \
        arr->data[arr->size++] = x;                                         \
    }

#endif

// vim: ft=c
