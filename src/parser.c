#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- Helpers ----

static void advance(Parser *p) {
    p->previous = p->current;
    p->current = lexer_next(&p->lexer);

    if (p->current.type == TOK_ERROR) {
        p->had_error = 1;
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "line %d: %.*s", p->current.line, p->current.length, p->current.start);
    }
}

static int check(Parser *p, TokenType type) {
    return p->current.type == type;
}

static int match(Parser *p, TokenType type) {
    if (check(p, type)) {
        advance(p);
        return 1;
    }
    return 0;
}

static void expect(Parser *p, TokenType type, const char *msg) {
    if (!match(p, type)) {
        p->had_error = 1;
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "line %d: expected %s, got '%s'",
                 p->current.line, msg, token_type_name(p->current.type));
        fprintf(stderr, "Parse error: %s\n", p->error_msg);
    }
}

static float expect_number(Parser *p) {
    expect(p, TOK_NUMBER, "number");
    return (float)p->previous.num_value;
}

static Color expect_color(Parser *p) {
    expect(p, TOK_HEX_COLOR, "hex color");
    return (Color){p->previous.color_r, p->previous.color_g, p->previous.color_b, 1.0f};
}

// ---- Property parsing (shared across all shapes) ----

static int parse_common_prop(Parser *p, ShapeProps *props) {
    if (match(p, TOK_POS)) {
        props->pos.x = expect_number(p);
        props->pos.y = expect_number(p);
        // optional z
        if (check(p, TOK_NUMBER)) {
            props->pos.z = expect_number(p);
        }
        props->has_pos = 1;
        return 1;
    }
    if (match(p, TOK_COLOR)) {
        props->color = expect_color(p);
        props->has_color = 1;
        return 1;
    }
    if (match(p, TOK_ROTATE)) {
        props->rotate.x = expect_number(p);
        if (check(p, TOK_NUMBER)) props->rotate.y = expect_number(p);
        if (check(p, TOK_NUMBER)) props->rotate.z = expect_number(p);
        props->has_rotate = 1;
        return 1;
    }
    if (match(p, TOK_SCALE)) {
        props->scale.x = expect_number(p);
        if (check(p, TOK_NUMBER)) props->scale.y = expect_number(p);
        if (check(p, TOK_NUMBER)) props->scale.z = expect_number(p);
        props->has_scale = 1;
        return 1;
    }
    if (match(p, TOK_OPACITY)) {
        props->opacity = expect_number(p);
        return 1;
    }
    if (match(p, TOK_FILL)) {
        if (match(p, TOK_SOLID)) props->fill = FILL_SOLID;
        else if (match(p, TOK_WIREFRAME)) props->fill = FILL_WIREFRAME;
        else {
            p->had_error = 1;
            snprintf(p->error_msg, sizeof(p->error_msg),
                     "line %d: expected 'solid' or 'wireframe'", p->current.line);
        }
        return 1;
    }
    return 0;
}

// ---- Shape parsers ----

static ASTNode *parse_circle(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_CIRCLE;
    node->data.circle.props = default_props();
    node->data.circle.radius = 50;

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_RADIUS)) {
            node->data.circle.radius = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.circle.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in circle\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_rect(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_RECT;
    node->data.rect.props = default_props();
    node->data.rect.size = (Vec2){100, 100};

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_SIZE)) {
            node->data.rect.size.x = expect_number(p);
            node->data.rect.size.y = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.rect.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in rect\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_line(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_LINE;
    node->data.line.props = default_props();
    node->data.line.width = 1.0f;

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_FROM)) {
            node->data.line.from.x = expect_number(p);
            node->data.line.from.y = expect_number(p);
        } else if (match(p, TOK_TO)) {
            node->data.line.to.x = expect_number(p);
            node->data.line.to.y = expect_number(p);
        } else if (match(p, TOK_WIDTH)) {
            node->data.line.width = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.line.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in line\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_triangle(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_TRIANGLE;
    node->data.triangle.props = default_props();

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_P1)) {
            node->data.triangle.p1.x = expect_number(p);
            node->data.triangle.p1.y = expect_number(p);
        } else if (match(p, TOK_P2)) {
            node->data.triangle.p2.x = expect_number(p);
            node->data.triangle.p2.y = expect_number(p);
        } else if (match(p, TOK_P3)) {
            node->data.triangle.p3.x = expect_number(p);
            node->data.triangle.p3.y = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.triangle.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in triangle\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_text(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_TEXT;
    node->data.text.props = default_props();
    node->data.text.font_size = 16;
    strcpy(node->data.text.content, "");

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_CONTENT)) {
            expect(p, TOK_STRING, "string");
            int len = p->previous.length < 255 ? p->previous.length : 255;
            memcpy(node->data.text.content, p->previous.start, len);
            node->data.text.content[len] = '\0';
        } else if (match(p, TOK_SIZE)) {
            node->data.text.font_size = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.text.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in text\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

// ---- 3D shapes ----

static ASTNode *parse_cube(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_CUBE;
    node->data.cube.props = default_props();
    node->data.cube.size = (Vec3){1, 1, 1};

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_SIZE)) {
            node->data.cube.size.x = expect_number(p);
            if (check(p, TOK_NUMBER)) node->data.cube.size.y = expect_number(p);
            else node->data.cube.size.y = node->data.cube.size.x;
            if (check(p, TOK_NUMBER)) node->data.cube.size.z = expect_number(p);
            else node->data.cube.size.z = node->data.cube.size.x;
        } else if (!parse_common_prop(p, &node->data.cube.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in cube\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_sphere(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_SPHERE;
    node->data.sphere.props = default_props();
    node->data.sphere.radius = 1.0f;

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_RADIUS)) {
            node->data.sphere.radius = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.sphere.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in sphere\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_pyramid(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_PYRAMID;
    node->data.pyramid.props = default_props();
    node->data.pyramid.base = 2.0f;
    node->data.pyramid.height = 3.0f;

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_BASE)) {
            node->data.pyramid.base = expect_number(p);
        } else if (match(p, TOK_HEIGHT)) {
            node->data.pyramid.height = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.pyramid.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in pyramid\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_plane_shape(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_PLANE;
    node->data.plane.props = default_props();
    node->data.plane.size = (Vec2){10, 10};

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_SIZE)) {
            node->data.plane.size.x = expect_number(p);
            node->data.plane.size.y = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.plane.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in plane\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_cylinder(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_CYLINDER;
    node->data.cylinder.props = default_props();
    node->data.cylinder.radius = 1.0f;
    node->data.cylinder.height = 2.0f;

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_RADIUS)) {
            node->data.cylinder.radius = expect_number(p);
        } else if (match(p, TOK_HEIGHT)) {
            node->data.cylinder.height = expect_number(p);
        } else if (!parse_common_prop(p, &node->data.cylinder.props)) {
            fprintf(stderr, "line %d: unexpected token '%s' in cylinder\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

// ---- Compound parsers ----

static ASTNode *parse_shape(Parser *p);

static ASTNode *parse_group(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_GROUP;
    node->data.group.props = default_props();
    node->data.group.children = NULL;
    node->data.group.child_count = 0;
    node->data.group.child_capacity = 0;
    strcpy(node->data.group.name, "");

    // optional name
    if (check(p, TOK_STRING)) {
        advance(p);
        int len = p->previous.length < 127 ? p->previous.length : 127;
        memcpy(node->data.group.name, p->previous.start, len);
        node->data.group.name[len] = '\0';
    }

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        // try common properties first
        if (parse_common_prop(p, &node->data.group.props)) {
            continue;
        }
        // try parsing a child shape
        ASTNode *child = parse_shape(p);
        if (child) {
            group_add_child(&node->data.group, child);
        } else {
            fprintf(stderr, "line %d: unexpected token '%s' in group\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_light(Parser *p) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = NODE_LIGHT;
    node->data.light.type = LIGHT_POINT;
    node->data.light.pos = (Vec3){0, 5, 5};
    node->data.light.color = (Color){1, 1, 1, 1};
    node->data.light.intensity = 1.0f;

    expect(p, TOK_LBRACE, "{");
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->had_error) {
        if (match(p, TOK_TYPE)) {
            if (match(p, TOK_POINT)) node->data.light.type = LIGHT_POINT;
            else if (match(p, TOK_DIRECTIONAL)) node->data.light.type = LIGHT_DIRECTIONAL;
            else if (match(p, TOK_AMBIENT)) node->data.light.type = LIGHT_AMBIENT;
        } else if (match(p, TOK_POS)) {
            node->data.light.pos.x = expect_number(p);
            node->data.light.pos.y = expect_number(p);
            node->data.light.pos.z = expect_number(p);
        } else if (match(p, TOK_COLOR)) {
            node->data.light.color = expect_color(p);
        } else if (match(p, TOK_INTENSITY)) {
            node->data.light.intensity = expect_number(p);
        } else {
            fprintf(stderr, "line %d: unexpected token '%s' in light\n",
                    p->current.line, token_type_name(p->current.type));
            advance(p);
        }
    }
    expect(p, TOK_RBRACE, "}");
    return node;
}

static ASTNode *parse_shape(Parser *p) {
    if (match(p, TOK_CIRCLE))   return parse_circle(p);
    if (match(p, TOK_RECT))     return parse_rect(p);
    if (match(p, TOK_LINE))     return parse_line(p);
    if (match(p, TOK_TRIANGLE)) return parse_triangle(p);
    if (match(p, TOK_TEXT))     return parse_text(p);
    if (match(p, TOK_CUBE))     return parse_cube(p);
    if (match(p, TOK_SPHERE))   return parse_sphere(p);
    if (match(p, TOK_PYRAMID))  return parse_pyramid(p);
    if (match(p, TOK_PLANE))    return parse_plane_shape(p);
    if (match(p, TOK_CYLINDER)) return parse_cylinder(p);
    if (match(p, TOK_GROUP))    return parse_group(p);
    if (match(p, TOK_LIGHT))    return parse_light(p);
    return NULL;
}

// ---- Top-level parser ----

static Scene *parse_internal(const char *source, int *out_had_error, char *out_error_msg, int error_msg_size) {
    Parser p;
    lexer_init(&p.lexer, source);
    p.had_error = 0;
    p.error_msg[0] = '\0';
    advance(&p); // prime the parser

    Scene *scene = scene_create();

    while (!check(&p, TOK_EOF)) {
        if (p.had_error) {
            fprintf(stderr, "Parse aborted: %s\n", p.error_msg);
            break;
        }

        // scene-level commands
        if (match(&p, TOK_CANVAS)) {
            scene->settings.width = (int)expect_number(&p);
            scene->settings.height = (int)expect_number(&p);
            if (check(&p, TOK_STRING)) {
                advance(&p);
                int len = p.previous.length < 255 ? p.previous.length : 255;
                memcpy(scene->settings.title, p.previous.start, len);
                scene->settings.title[len] = '\0';
            }
            continue;
        }
        if (match(&p, TOK_BACKGROUND)) {
            scene->settings.background = expect_color(&p);
            continue;
        }
        if (match(&p, TOK_CAMERA)) {
            scene->settings.camera.x = expect_number(&p);
            scene->settings.camera.y = expect_number(&p);
            scene->settings.camera.z = expect_number(&p);
            scene->settings.has_camera = 1;
            continue;
        }

        // shapes and lights
        ASTNode *node = parse_shape(&p);
        if (node) {
            scene_add_node(scene, node);
        } else {
            fprintf(stderr, "line %d: unexpected top-level token '%s'\n",
                    p.current.line, token_type_name(p.current.type));
            advance(&p);
        }
    }

    if (out_had_error) *out_had_error = p.had_error;
    if (out_error_msg && error_msg_size > 0) {
        snprintf(out_error_msg, error_msg_size, "%s", p.error_msg);
    }

    if (p.had_error) {
        fprintf(stderr, "Scene parsed with errors.\n");
    } else {
        fprintf(stderr, "Scene parsed: %d nodes, canvas %dx%d\n",
                scene->node_count, scene->settings.width, scene->settings.height);
    }

    return scene;
}

Scene *parse(const char *source) {
    return parse_internal(source, NULL, NULL, 0);
}

Scene *parse_with_error(const char *source, int *out_had_error, char *out_error_msg, int error_msg_size) {
    return parse_internal(source, out_had_error, out_error_msg, error_msg_size);
}
