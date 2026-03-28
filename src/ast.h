#ifndef AST_H
#define AST_H

#include <stddef.h>

// ---- Math types ----

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;
typedef struct { float r, g, b, a; } Color;

// ---- Node types ----

typedef enum {
    NODE_SCENE,
    NODE_CIRCLE,
    NODE_RECT,
    NODE_LINE,
    NODE_TRIANGLE,
    NODE_TEXT,
    NODE_CUBE,
    NODE_SPHERE,
    NODE_PYRAMID,
    NODE_PLANE,
    NODE_CYLINDER,
    NODE_GROUP,
    NODE_LIGHT
} NodeType;

typedef enum {
    FILL_SOLID,
    FILL_WIREFRAME
} FillMode;

typedef enum {
    LIGHT_POINT,
    LIGHT_DIRECTIONAL,
    LIGHT_AMBIENT
} LightType;

// ---- Shape properties (common to all shapes) ----

typedef struct {
    Vec3 pos;
    Vec3 rotate;
    Vec3 scale;
    Color color;
    float opacity;
    FillMode fill;
    int has_pos;
    int has_rotate;
    int has_scale;
    int has_color;
} ShapeProps;

// ---- Individual shape data ----

typedef struct {
    ShapeProps props;
    float radius;
} CircleNode;

typedef struct {
    ShapeProps props;
    Vec2 size;
} RectNode;

typedef struct {
    ShapeProps props;
    Vec2 from;
    Vec2 to;
    float width;
} LineNode;

typedef struct {
    ShapeProps props;
    Vec2 p1, p2, p3;
} TriangleNode;

typedef struct {
    ShapeProps props;
    char content[256];
    float font_size;
} TextNode;

typedef struct {
    ShapeProps props;
    Vec3 size;
} CubeNode;

typedef struct {
    ShapeProps props;
    float radius;
} SphereNode;

typedef struct {
    ShapeProps props;
    float base;
    float height;
} PyramidNode;

typedef struct {
    ShapeProps props;
    Vec2 size;
} PlaneNode;

typedef struct {
    ShapeProps props;
    float radius;
    float height;
} CylinderNode;

// ---- Forward declare ASTNode ----

typedef struct ASTNode ASTNode;

typedef struct {
    ShapeProps props;
    char name[128];
    ASTNode **children;
    int child_count;
    int child_capacity;
} GroupNode;

typedef struct {
    LightType type;
    Vec3 pos;
    Color color;
    float intensity;
} LightNode;

// ---- Scene settings ----

typedef struct {
    int width, height;
    char title[256];
    Color background;
    Vec3 camera;
    int has_camera;
} SceneSettings;

// ---- Generic AST Node (tagged union) ----

struct ASTNode {
    NodeType type;
    union {
        CircleNode   circle;
        RectNode     rect;
        LineNode     line;
        TriangleNode triangle;
        TextNode     text;
        CubeNode     cube;
        SphereNode   sphere;
        PyramidNode  pyramid;
        PlaneNode    plane;
        CylinderNode cylinder;
        GroupNode    group;
        LightNode    light;
    } data;
};

// ---- Scene: the root of the AST ----

typedef struct {
    SceneSettings settings;
    ASTNode **nodes;
    int node_count;
    int node_capacity;
} Scene;

// ---- Helper functions ----

static inline ShapeProps default_props(void) {
    ShapeProps p = {0};
    p.pos = (Vec3){0, 0, 0};
    p.rotate = (Vec3){0, 0, 0};
    p.scale = (Vec3){1, 1, 1};
    p.color = (Color){1, 1, 1, 1};
    p.opacity = 1.0f;
    p.fill = FILL_SOLID;
    p.has_pos = 0;
    p.has_rotate = 0;
    p.has_scale = 0;
    p.has_color = 0;
    return p;
}

Scene *scene_create(void);
void   scene_add_node(Scene *scene, ASTNode *node);
void   group_add_child(GroupNode *group, ASTNode *node);
void   scene_free(Scene *scene);

#endif
