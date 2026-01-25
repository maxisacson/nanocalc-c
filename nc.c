#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "evaler.h"

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        exit(1);
    }

    struct Token* tokens;
    tokenize(argv[1], &tokens);

    struct AstNode* root = new_node();
    struct Parser parser;
    parser.tokens = tokens;
    parser.tok = tokens;

    parse(&parser, root);

    while (tokens->type != TOK_EOF) {
        printf("%s ", tok_to_str(*tokens++));
    }

    printf("%s ", tok_to_str(*tokens++));
    printf("\n");

    printf("%s\n", ast_node_to_str(root));

    struct Context context = new_context(NULL);

    struct AstValue val = {.type=V_INT, .int_value = 42};
    set_value(&context, "x", val);

    struct AstValue result = eval(root, &context);

    const char* out = ast_value_to_str(&result);
    printf("%s\n", out);

    draw_ast(root);

    return 0;
}
