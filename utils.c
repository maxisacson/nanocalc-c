#include "utils.h"
#include <string.h>

void sb_append(StringBuilder* sb, const char* str) {
    if (sb->capacity == 0) {
        sb->capacity = 16;
        sb->data = malloc(sb->capacity * sizeof(const char*));
    }

    size_t cap = sb->capacity;
    while (sb->size >= cap) {
        cap *= 2;
    }

    if (cap != sb->capacity) {
        sb->capacity = cap;
        sb->data = realloc(sb->data, sb->capacity);
    }

    sb->data[sb->size++] = str;
}

StringBuilder sb_join(const char* sep, const char** strs, size_t strc) {
    StringBuilder sb = {};

    for (size_t i = 0; i < strc; ++i) {
        if (i > 0) {
            sb_append(&sb, sep);
        }
        sb_append(&sb, strs[i]);
    }

    return sb;
}

void sb_append_v(StringBuilder* sb, size_t count, const char* strings[]) {
    for (size_t i = 0; i < count; ++i) {
        sb_append(sb, strings[i]);
    }
}

const char* sb_string(StringBuilder* sb) {
    size_t total = 0;
    for (size_t i = 0; i < sb->size; ++i) {
        total += strlen(sb->data[i]);
    }

    char* str = malloc((total + 1) * sizeof(char));
    char* pos = str;
    for (size_t i = 0; i < sb->size; ++i) {
        size_t len = strlen(sb->data[i]);
        memcpy(pos, sb->data[i], len);
        pos += len;
    }
    *pos = '\0';
    return str;
}

void ptrarr_append(PtrArr* a, void* data) {
    if (a->capacity == 0) {
        a->capacity = 32;
        a->data = malloc(a->capacity * sizeof(void*));
    }

    size_t cap = a->capacity;
    while (cap <= a->size) {
        cap *= 2;
    }

    if (cap != a->capacity) {
        a->capacity = cap;
        a->data = realloc(a->data, a->capacity * sizeof(void*));
    }

    a->data[a->size++] = data;
}
