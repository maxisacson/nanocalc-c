#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "evaler.h"

#define BUFSIZE 4095 // pagesize - 1

const char* read_file(FILE* fd) {
    char buf[BUFSIZE];
    size_t bytes;
    size_t total = 0;
    char* contents;
    while ((bytes = fread(buf, 1, BUFSIZE, fd)) > 0) {
        if (total == 0) {
            contents = malloc(bytes + 1);
        } else {
            contents = realloc(contents, total + bytes + 1);
        }
        memcpy(contents + total, buf, bytes);
        total += bytes;
    }
    contents[total] = 0;
    return contents;
}

int main(int argc, const char* argv[]) {
    const char* text;

    if (argc < 2) {
        text = read_file(stdin);
    } else {
        text = argv[1];
    }

    struct Token* tokens;
    tokenize(text, &tokens);

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

    struct Context context = context_new(NULL);

    struct AstValue val = {.type=V_INT, .int_value = 42};
    set_value(&context, "x", val);

    struct AstValue result = eval(&root, &context);

    const char* out = ast_value_to_str(&result);
    printf("%s\n", out);

    draw_ast(&root);

    return 0;
}
