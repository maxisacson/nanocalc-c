#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdlib.h>

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} String;

void string_append(String* s, const char* str);
void string_append_ch(String* s, char c);
void string_reserve(String* s, size_t size);
void string_set(String* s, const char* str);
String string_join(const char* sep, const char** strs, size_t strc);

#endif
