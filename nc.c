#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "nc_types.h"

#include "lexer.h"
#include "parser.h"
#include "evaler.h"
#include "version.h"

#define BUFSIZE 4095  // pagesize - 1

// Command line option flags
bool FLAG_DEBUG = false;
bool FLAG_EMIT_TOKENS = false;
bool FLAG_EMIT_AST = false;

const char* read_file(FILE* fd) {
    char buf[BUFSIZE];
    size_t bytes;
    size_t total = 0;
    char* contents = NULL;
    while ((bytes = fread(buf, 1, BUFSIZE, fd)) > 0) {
        if (total == 0) {
            contents = malloc(bytes + 1);
        } else {
            contents = realloc(contents, total + bytes + 1);
        }
        memcpy(contents + total, buf, bytes);
        total += bytes;
    }
    if (contents) {
        contents[total] = 0;
    }
    return contents;
}

bool get_line(char** text) {
    char buf[BUFSIZE];

    ssize_t bytes = read(1, buf, BUFSIZE);
    if (bytes < 0) {
        exit(EXIT_FAILURE);
    } else if (bytes == 0) {
        return false;
    }

    *text = malloc(bytes + 1);
    memcpy(*text, buf, bytes);
    (*text)[bytes] = 0;

    return true;
}

void repl() {
    char* text = NULL;

    struct Context builtin = context_new(NULL);
    setup_builtin_context(&builtin);
    struct Context context = context_new(&builtin);

    printf("nc> ");
    fflush(stdout);
    while (get_line(&text)) {
        struct Lexer lexer = lexer_new(text);
        struct Token* tokens;
        tokenize(&lexer, &tokens);

        struct Parser parser;
        parser.tokens = tokens;
        parser.tok = tokens;

        struct AstNode root;
        parse(&parser, &root);

        struct AstValue result = eval(&root, &context);

        if (result.type != V_NIL) {
            const char* out = ast_value_to_str(&result);
            printf("%s\n", out);
        }
        printf("nc> ");
        fflush(stdout);
    }
    // TODO: check eof and errors
}

void usage(const char* prog) {
    printf("Usage: %s [options] [expression]\n", prog);
    printf("\n");
    printf("If expression is not given, read from standard input\n");
    printf("\n");
    printf("Options:\n"
           "     -h|--help     Display this message and exit\n"
           "     -V|--version  Display version info and exit\n"
           "     --debug       Enable debug output\n"
           "     --tokens      Emit the token stream to stdout\n"
           "     --ast         Emit the AST to file\n"
          );
}

void print_version() {
#ifdef DEBUG
    printf("nanocalc v%d.%d.%d-%s+%d (debug)\n", NC_VERSION_MAJOR, NC_VERSION_MINOR, NC_VERSION_PATCH, NC_VERSION_PRERELEASE, NC_VERSION_BUILD);
#else
    printf("nanocalc v%d.%d.%d\n", NC_VERSION_MAJOR, NC_VERSION_MINOR, NC_VERSION_PATCH);
#endif
}

int parse_args(int argc, char* argv[]) {
    int opt, longindex;
    for (;;) {
        // clang-format off
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"debug", no_argument, 0, 0},
            {"tokens", no_argument, 0, 0},
            {"ast", no_argument, 0, 0},
            {"version", no_argument, 0, 'V'},
            {0, 0, 0, 0},
        };
        // clang-format on

        opt = getopt_long(argc, argv, "hV", long_options, &longindex);

        if (opt == -1) {
            break;
        }

        switch (opt) {
            case 0:
                if (strcmp(long_options[longindex].name, "debug") == 0) {
                    FLAG_DEBUG = true;
                } else if (strcmp(long_options[longindex].name, "ast") == 0) {
                    FLAG_EMIT_AST = true;
                } else if (strcmp(long_options[longindex].name, "tokens") == 0) {
                    FLAG_EMIT_TOKENS = true;
                }
                break;
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case 'V':
                print_version();
                exit(EXIT_SUCCESS);
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (FLAG_DEBUG) {
        FLAG_EMIT_AST = true;
        FLAG_EMIT_TOKENS = true;
    }

    return optind;
}

int main(int argc, char* argv[]) {
    int iarg = parse_args(argc, argv);

    const char* text;
    if (iarg == argc) {
        if (isatty(STDIN_FILENO)) {
            repl();
            return EXIT_SUCCESS;
        } else {
            text = read_file(stdin);
        }
    } else if (iarg < argc) {
        text = argv[iarg];
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    struct Lexer lexer = lexer_new(text);
    struct Token* tokens;
    tokenize(&lexer, &tokens);

    struct Parser parser;
    parser.tokens = tokens;
    parser.tok = tokens;

    if (FLAG_EMIT_TOKENS) {
        while (tokens->type != TOK_EOF) {
            printf("%s ", tok_to_str(*tokens++));
        }

        printf("%s ", tok_to_str(*tokens++));
        printf("\n");
    }

    struct AstNode root;
    parse(&parser, &root);

    if (FLAG_EMIT_AST) {
        draw_ast(&root);
    }

    struct Context builtin = context_new(NULL);
    setup_builtin_context(&builtin);
    struct Context context = context_new(&builtin);

    struct AstValue result = eval(&root, &context);

    if (result.type != V_NIL) {
        const char* out = ast_value_to_str(&result);
        printf("%s\n", out);
    }

    return EXIT_SUCCESS;
}
