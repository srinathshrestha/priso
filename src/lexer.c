#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
}

static void skip_whitespace_and_comments(Lexer *lexer) {
    for (;;) {
        char c = *lexer->current;
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->current++;
        } else if (c == '\n') {
            lexer->line++;
            lexer->current++;
        } else if (c == '#' && !isxdigit(lexer->current[1])) {
            // line comment — # followed by non-hex char
            while (*lexer->current && *lexer->current != '\n')
                lexer->current++;
        } else if (c == '/' && lexer->current[1] == '/') {
            // also support // comments
            while (*lexer->current && *lexer->current != '\n')
                lexer->current++;
        } else {
            break;
        }
    }
}

static Token make_token(Lexer *lexer, TokenType type, const char *start, int length) {
    Token tok;
    tok.type = type;
    tok.start = start;
    tok.length = length;
    tok.line = lexer->line;
    tok.num_value = 0;
    tok.color_r = tok.color_g = tok.color_b = 0;
    return tok;
}

static Token error_token(Lexer *lexer, const char *msg) {
    Token tok;
    tok.type = TOK_ERROR;
    tok.start = msg;
    tok.length = (int)strlen(msg);
    tok.line = lexer->line;
    tok.num_value = 0;
    tok.color_r = tok.color_g = tok.color_b = 0;
    return tok;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static Token lex_hex_color(Lexer *lexer) {
    const char *start = lexer->current;
    lexer->current++; // skip #

    const char *hex_start = lexer->current;
    while (isxdigit(*lexer->current))
        lexer->current++;

    int hex_len = (int)(lexer->current - hex_start);
    if (hex_len != 6) {
        return error_token(lexer, "hex color must be 6 digits (#rrggbb)");
    }

    Token tok = make_token(lexer, TOK_HEX_COLOR, start, (int)(lexer->current - start));

    tok.color_r = (hex_digit(hex_start[0]) * 16 + hex_digit(hex_start[1])) / 255.0f;
    tok.color_g = (hex_digit(hex_start[2]) * 16 + hex_digit(hex_start[3])) / 255.0f;
    tok.color_b = (hex_digit(hex_start[4]) * 16 + hex_digit(hex_start[5])) / 255.0f;

    return tok;
}

static Token lex_number(Lexer *lexer) {
    const char *start = lexer->current;

    // handle negative numbers
    if (*lexer->current == '-')
        lexer->current++;

    while (isdigit(*lexer->current))
        lexer->current++;

    if (*lexer->current == '.' && isdigit(lexer->current[1])) {
        lexer->current++; // skip dot
        while (isdigit(*lexer->current))
            lexer->current++;
    }

    Token tok = make_token(lexer, TOK_NUMBER, start, (int)(lexer->current - start));
    tok.num_value = strtod(start, NULL);
    return tok;
}

static Token lex_string(Lexer *lexer) {
    lexer->current++; // skip opening "
    const char *start = lexer->current;

    while (*lexer->current && *lexer->current != '"') {
        if (*lexer->current == '\n') lexer->line++;
        lexer->current++;
    }

    if (*lexer->current == '\0') {
        return error_token(lexer, "unterminated string");
    }

    Token tok = make_token(lexer, TOK_STRING, start, (int)(lexer->current - start));
    lexer->current++; // skip closing "
    return tok;
}

typedef struct {
    const char *name;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    // Scene
    {"canvas",      TOK_CANVAS},
    {"background",  TOK_BACKGROUND},
    {"camera",      TOK_CAMERA},
    {"light",       TOK_LIGHT},
    {"group",       TOK_GROUP},

    // 2D
    {"circle",      TOK_CIRCLE},
    {"rect",        TOK_RECT},
    {"line",        TOK_LINE},
    {"triangle",    TOK_TRIANGLE},
    {"text",        TOK_TEXT},

    // 3D
    {"cube",        TOK_CUBE},
    {"sphere",      TOK_SPHERE},
    {"pyramid",     TOK_PYRAMID},
    {"plane",       TOK_PLANE},
    {"cylinder",    TOK_CYLINDER},

    // Properties
    {"pos",         TOK_POS},
    {"radius",      TOK_RADIUS},
    {"size",        TOK_SIZE},
    {"color",       TOK_COLOR},
    {"rotate",      TOK_ROTATE},
    {"scale",       TOK_SCALE},
    {"opacity",     TOK_OPACITY},
    {"fill",        TOK_FILL},
    {"from",        TOK_FROM},
    {"to",          TOK_TO},
    {"width",       TOK_WIDTH},
    {"height",      TOK_HEIGHT},
    {"base",        TOK_BASE},
    {"content",     TOK_CONTENT},
    {"type",        TOK_TYPE},
    {"intensity",   TOK_INTENSITY},
    {"p1",          TOK_P1},
    {"p2",          TOK_P2},
    {"p3",          TOK_P3},

    // Fill types
    {"solid",       TOK_SOLID},
    {"wireframe",   TOK_WIREFRAME},

    // Light types
    {"point",       TOK_POINT},
    {"directional", TOK_DIRECTIONAL},
    {"ambient",     TOK_AMBIENT},

    {NULL, TOK_EOF}
};

static Token lex_identifier(Lexer *lexer) {
    const char *start = lexer->current;

    while (isalnum(*lexer->current) || *lexer->current == '_')
        lexer->current++;

    int length = (int)(lexer->current - start);

    // check keywords
    for (int i = 0; keywords[i].name != NULL; i++) {
        if ((int)strlen(keywords[i].name) == length &&
            memcmp(start, keywords[i].name, length) == 0) {
            return make_token(lexer, keywords[i].type, start, length);
        }
    }

    return make_token(lexer, TOK_IDENTIFIER, start, length);
}

Token lexer_next(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    if (*lexer->current == '\0') {
        return make_token(lexer, TOK_EOF, lexer->current, 0);
    }

    char c = *lexer->current;

    // hex color: # followed by hex digit
    if (c == '#' && isxdigit(lexer->current[1])) {
        return lex_hex_color(lexer);
    }

    // number (including negative)
    if (isdigit(c) || (c == '-' && isdigit(lexer->current[1]))) {
        return lex_number(lexer);
    }

    // string
    if (c == '"') {
        return lex_string(lexer);
    }

    // identifier / keyword
    if (isalpha(c) || c == '_') {
        return lex_identifier(lexer);
    }

    // single char tokens
    if (c == '{') {
        const char *start = lexer->current++;
        return make_token(lexer, TOK_LBRACE, start, 1);
    }
    if (c == '}') {
        const char *start = lexer->current++;
        return make_token(lexer, TOK_RBRACE, start, 1);
    }

    return error_token(lexer, "unexpected character");
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_NUMBER:      return "NUMBER";
        case TOK_STRING:      return "STRING";
        case TOK_HEX_COLOR:   return "HEX_COLOR";
        case TOK_LBRACE:      return "{";
        case TOK_RBRACE:      return "}";
        case TOK_CANVAS:      return "canvas";
        case TOK_BACKGROUND:  return "background";
        case TOK_CAMERA:      return "camera";
        case TOK_LIGHT:       return "light";
        case TOK_GROUP:       return "group";
        case TOK_CIRCLE:      return "circle";
        case TOK_RECT:        return "rect";
        case TOK_LINE:        return "line";
        case TOK_TRIANGLE:    return "triangle";
        case TOK_TEXT:        return "text";
        case TOK_CUBE:        return "cube";
        case TOK_SPHERE:      return "sphere";
        case TOK_PYRAMID:     return "pyramid";
        case TOK_PLANE:       return "plane";
        case TOK_CYLINDER:    return "cylinder";
        case TOK_POS:         return "pos";
        case TOK_RADIUS:      return "radius";
        case TOK_SIZE:        return "size";
        case TOK_COLOR:       return "color";
        case TOK_ROTATE:      return "rotate";
        case TOK_SCALE:       return "scale";
        case TOK_OPACITY:     return "opacity";
        case TOK_FILL:        return "fill";
        case TOK_FROM:        return "from";
        case TOK_TO:          return "to";
        case TOK_WIDTH:       return "width";
        case TOK_HEIGHT:      return "height";
        case TOK_BASE:        return "base";
        case TOK_CONTENT:     return "content";
        case TOK_TYPE:        return "type";
        case TOK_INTENSITY:   return "intensity";
        case TOK_P1:          return "p1";
        case TOK_P2:          return "p2";
        case TOK_P3:          return "p3";
        case TOK_SOLID:       return "solid";
        case TOK_WIREFRAME:   return "wireframe";
        case TOK_POINT:       return "point";
        case TOK_DIRECTIONAL: return "directional";
        case TOK_AMBIENT:     return "ambient";
        case TOK_IDENTIFIER:  return "IDENTIFIER";
        case TOK_EOF:         return "EOF";
        case TOK_ERROR:       return "ERROR";
    }
    return "UNKNOWN";
}
