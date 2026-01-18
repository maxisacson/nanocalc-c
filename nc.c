#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        exit(1);
    }

    struct Token* tokens;
    tokenize(argv[1], &tokens);

    while (tokens->type != END) {
        printf("%s ", tok_to_str(*tokens++));
    }

    printf("%s ", tok_to_str(*tokens++));
    printf("\n");

    return 0;
}
