#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nc_types.h"

#include "lexer.h"
#include "parser.h"
#include "evaler.h"

#define BUFSIZE 4095  // pagesize - 1

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

    struct Lexer lexer;
    lexer.line = 1;
    lexer.col = 0;

    printf("nc> ");
    fflush(stdout);
    while (get_line(&text)) {
        struct Token* tokens;
        tokenize(&lexer, text, &tokens);

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

int main(int argc, const char* argv[]) {
    if (argc == 1 && isatty(STDIN_FILENO)) {
        repl();
        return EXIT_SUCCESS;
    }

    const char* text;

    if (argc < 2) {
        text = read_file(stdin);
    } else {
        text = argv[1];
    }

    struct Lexer lexer;
    lexer.line = 1;
    lexer.col = 0;
    struct Token* tokens;
    tokenize(&lexer, text, &tokens);

    struct Parser parser;
    parser.tokens = tokens;
    parser.tok = tokens;

    while (tokens->type != TOK_EOF) {
        printf("%s ", tok_to_str(*tokens++));
    }

    printf("%s ", tok_to_str(*tokens++));
    printf("\n");

    struct AstNode root;
    parse(&parser, &root);

    draw_ast(&root);

    struct Context builtin = context_new(NULL);
    setup_builtin_context(&builtin);
    struct Context context = context_new(&builtin);

    struct AstValue val = {.type = V_INT, .int_value = 42};
    set_value(&context, "x", val);

    struct AstValue result = eval(&root, &context);

    if (result.type != V_NIL) {
        const char* out = ast_value_to_str(&result);
        printf("%s\n", out);
    }

    return EXIT_SUCCESS;
}
