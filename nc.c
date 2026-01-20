#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        exit(1);
    }

    struct Token* tokens;
    tokenize(argv[1], &tokens);

    struct AstNode root;
    struct Parser parser;
    parser.tokens = tokens;
    parser.tok = tokens;
    parse_expr(&parser, &root);

    while (tokens->type != TOK_EOF) {
        printf("%s ", tok_to_str(*tokens++));
    }

    printf("%s ", tok_to_str(*tokens++));
    printf("\n");

    printf("%s\n", ast_node_to_str(root));

    return 0;
}
