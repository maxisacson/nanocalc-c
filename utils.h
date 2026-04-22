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

#endif

// vim: ft=c
