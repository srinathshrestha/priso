#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    int had_error;
    char error_msg[512];
} Parser;

Scene *parse(const char *source);
Scene *parse_with_error(const char *source, int *out_had_error, char *out_error_msg, int error_msg_size);

#endif
