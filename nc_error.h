#ifndef NC_ERROR_H
#define NC_ERROR_H

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#include <execinfo.h>
#include <unistd.h>

#define lineno() fprintf(stderr, "%s:%d -> %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)

#define BT_SIZE 1024
#define print_backtrace()                                  \
    do {                                                   \
        char bt[BT_SIZE] = {};                             \
        int n = backtrace((void*)bt, BT_SIZE);             \
        if (n < 0) {                                       \
            break;                                         \
        }                                                  \
        backtrace_symbols_fd((void*)bt, n, STDOUT_FILENO); \
        printf("++++++++++++++++++++++++\n");              \
    } while (0)

#else
#define lineno()
#define print_backtrace()
#endif

#define error(...)                          \
    print_backtrace();                      \
    lineno();                               \
    fprintf(stderr, "error: " __VA_ARGS__); \
    exit(1)

#define syntax_error(tok, fmt, ...)                                                                        \
    print_backtrace();                                                                                     \
    lineno();                                                                                              \
    fprintf(stderr, "syntax_error: " fmt " (line %d, column %d)\n", __VA_ARGS__, (tok)->line, (tok)->col); \
    exit(1)

#define eval_error(...)                          \
    print_backtrace();                           \
    lineno();                                    \
    fprintf(stderr, "eval_error: " __VA_ARGS__); \
    exit(1)

#define incompatible_types(typea, typeb) \
    eval_error("incompatible types: %s and %s\n", value_type_to_str(typea), value_type_to_str(typeb));

#define unreachable_code()                                                                     \
    fprintf(stderr, "unreachable_code: %s: %s:%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    abort()

#endif
