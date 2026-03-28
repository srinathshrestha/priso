#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
    // Literals
    TOK_NUMBER,         // 42, 3.14
    TOK_STRING,         // "hello"
    TOK_HEX_COLOR,      // #ff3344

    // Symbols
    TOK_LBRACE,         // {
    TOK_RBRACE,         // }

    // Scene-level keywords
    TOK_CANVAS,
    TOK_BACKGROUND,
    TOK_CAMERA,
    TOK_LIGHT,
    TOK_GROUP,

    // 2D shape keywords
    TOK_CIRCLE,
    TOK_RECT,
    TOK_LINE,
    TOK_TRIANGLE,
    TOK_TEXT,

    // 3D shape keywords
    TOK_CUBE,
    TOK_SPHERE,
    TOK_PYRAMID,
    TOK_PLANE,
    TOK_CYLINDER,

    // Property keywords
    TOK_POS,
    TOK_RADIUS,
    TOK_SIZE,
    TOK_COLOR,
    TOK_ROTATE,
    TOK_SCALE,
    TOK_OPACITY,
    TOK_FILL,
    TOK_FROM,
    TOK_TO,
    TOK_WIDTH,
    TOK_HEIGHT,
    TOK_BASE,
    TOK_CONTENT,
    TOK_TYPE,
    TOK_INTENSITY,
    TOK_P1,
    TOK_P2,
    TOK_P3,

    // Fill types
    TOK_SOLID,
    TOK_WIREFRAME,

    // Light types
    TOK_POINT,
    TOK_DIRECTIONAL,
    TOK_AMBIENT,

    // Identifier (catch-all for unknown words)
    TOK_IDENTIFIER,

    // Special
    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char *start;      // pointer into source
    int length;
    int line;

    // parsed values
    double num_value;
    float color_r, color_g, color_b;
} Token;

typedef struct {
    const char *source;
    const char *current;
    int line;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next(Lexer *lexer);
const char *token_type_name(TokenType type);

#endif
