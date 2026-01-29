#include "utils.h"
#include <string.h>

void string_reserve_exact(String* s, size_t size) {
    if (s->capacity == 0) {
        s->capacity = 32;
        s->data = (char*)calloc(s->capacity, 0);
    } else if (s->capacity < size + 1) {
        s->capacity = size + 1;
        s->data = realloc(s->data, s->capacity);
        memset(s->data + s->size, 0, s->capacity - s->size);
    }
}

void string_set(String* s, const char* str) {
    size_t len = strlen(str);
    string_reserve_exact(s, len);
    memcpy(s->data, str, len);
    s->size = len;
    s->data[len] = 0;
}

void string_append(String* s, const char* str) {
    size_t len = strlen(str);
    string_reserve(s, s->size + len);
    memcpy(s->data + s->size, str, len);
    s->size += len;
}

void string_append_ch(String* s, char c) {
    string_reserve(s, s->size + 1);
    s->data[s->size++] = c;
}

void string_reserve(String* s, size_t size) {
    if (s->capacity == 0) {
        s->capacity = 32;
        s->data = (char*)calloc(s->capacity, 0);
    }

    size_t cap = s->capacity;
    while (cap < size + 1) {
        cap *= 2;
    }

    if (cap != s->capacity) {
        s->capacity = cap;
        s->data = realloc(s->data, s->capacity);
        memset(s->data + s->size, 0, s->capacity - s->size);
    }
}

String string_join(const char* sep, const char** strs, size_t strc) {
    size_t total = 0;
    for (size_t i = 0; i < strc; ++i) {
        total += strlen(strs[i]);
    }
    total += (strc - 1) * strlen(sep);

    String s = {};
    string_reserve_exact(&s, total);

    for (size_t i = 0; i < strc; ++i) {
        if (i > 0) {
            string_append(&s, sep);
        }
        string_append(&s, strs[i]);
    }

    return s;
}
