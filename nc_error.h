#ifndef NC_ERROR_H
#define NC_ERROR_H

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

#endif
