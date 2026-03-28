#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ---- Embedded Shaders ----

static const char *vert_3d_src =
    "#version 330 core\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNormal;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProj;\n"
    "out vec3 vNormal;\n"
    "out vec3 vFragPos;\n"
    "void main() {\n"
    "    vec4 worldPos = uModel * vec4(aPos, 1.0);\n"
    "    vFragPos = worldPos.xyz;\n"
    "    vNormal = mat3(transpose(inverse(uModel))) * aNormal;\n"
    "    gl_Position = uProj * uView * worldPos;\n"
    "}\n";

static const char *frag_3d_src =
    "#version 330 core\n"
    "in vec3 vNormal;\n"
    "in vec3 vFragPos;\n"
    "uniform vec3 uColor;\n"
    "uniform float uOpacity;\n"
    "uniform vec3 uLightDir;\n"
    "uniform vec3 uLightColor;\n"
    "uniform float uAmbient;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    vec3 norm = normalize(vNormal);\n"
    "    float diff = max(dot(norm, normalize(uLightDir)), 0.0);\n"
    "    vec3 result = (uAmbient + diff) * uLightColor * uColor;\n"
    "    FragColor = vec4(result, uOpacity);\n"
    "}\n";

static const char *vert_2d_src =
    "#version 330 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "uniform mat4 uProj;\n"
    "uniform mat4 uModel;\n"
    "void main() {\n"
    "    gl_Position = uProj * uModel * vec4(aPos, 0.0, 1.0);\n"
    "}\n";

static const char *frag_2d_src =
    "#version 330 core\n"
    "uniform vec3 uColor;\n"
    "uniform float uOpacity;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(uColor, uOpacity);\n"
    "}\n";

// ---- Shader compilation ----

static unsigned int compile_shader(const char *src, GLenum type) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
    }
    return s;
}

static unsigned int link_program(const char *vert_src, const char *frag_src) {
    unsigned int v = compile_shader(vert_src, GL_VERTEX_SHADER);
    unsigned int f = compile_shader(frag_src, GL_FRAGMENT_SHADER);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    int ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, NULL, log);
        fprintf(stderr, "Shader link error: %s\n", log);
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// ---- Mesh generation ----

// Cube: 36 vertices (6 faces * 2 triangles * 3 verts) with normals
static void gen_cube(Renderer *r) {
    float v[] = {
        // pos              // normal
        // front
        -0.5f,-0.5f, 0.5f,  0,0,1,   0.5f,-0.5f, 0.5f,  0,0,1,   0.5f, 0.5f, 0.5f,  0,0,1,
        -0.5f,-0.5f, 0.5f,  0,0,1,   0.5f, 0.5f, 0.5f,  0,0,1,  -0.5f, 0.5f, 0.5f,  0,0,1,
        // back
        -0.5f,-0.5f,-0.5f,  0,0,-1,   0.5f, 0.5f,-0.5f,  0,0,-1,   0.5f,-0.5f,-0.5f,  0,0,-1,
        -0.5f,-0.5f,-0.5f,  0,0,-1,  -0.5f, 0.5f,-0.5f,  0,0,-1,   0.5f, 0.5f,-0.5f,  0,0,-1,
        // left
        -0.5f,-0.5f,-0.5f, -1,0,0,  -0.5f,-0.5f, 0.5f, -1,0,0,  -0.5f, 0.5f, 0.5f, -1,0,0,
        -0.5f,-0.5f,-0.5f, -1,0,0,  -0.5f, 0.5f, 0.5f, -1,0,0,  -0.5f, 0.5f,-0.5f, -1,0,0,
        // right
         0.5f,-0.5f,-0.5f,  1,0,0,   0.5f, 0.5f, 0.5f,  1,0,0,   0.5f,-0.5f, 0.5f,  1,0,0,
         0.5f,-0.5f,-0.5f,  1,0,0,   0.5f, 0.5f,-0.5f,  1,0,0,   0.5f, 0.5f, 0.5f,  1,0,0,
        // top
        -0.5f, 0.5f, 0.5f,  0,1,0,   0.5f, 0.5f, 0.5f,  0,1,0,   0.5f, 0.5f,-0.5f,  0,1,0,
        -0.5f, 0.5f, 0.5f,  0,1,0,   0.5f, 0.5f,-0.5f,  0,1,0,  -0.5f, 0.5f,-0.5f,  0,1,0,
        // bottom
        -0.5f,-0.5f, 0.5f,  0,-1,0,   0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f, 0.5f,  0,-1,0,
        -0.5f,-0.5f, 0.5f,  0,-1,0,  -0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f,-0.5f,  0,-1,0,
    };

    glGenVertexArrays(1, &r->cube_vao);
    glGenBuffers(1, &r->cube_vbo);
    glBindVertexArray(r->cube_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    r->cube_index_count = 36;
}

// Sphere: UV sphere with stacks and sectors
static void gen_sphere(Renderer *r) {
    int stacks = 20, sectors = 20;
    int vert_count = (stacks + 1) * (sectors + 1);
    int idx_count = stacks * sectors * 6;

    float *verts = malloc(vert_count * 6 * sizeof(float));
    unsigned int *indices = malloc(idx_count * sizeof(unsigned int));

    int vi = 0;
    for (int i = 0; i <= stacks; i++) {
        float phi = M_PI * i / stacks;
        for (int j = 0; j <= sectors; j++) {
            float theta = 2.0f * M_PI * j / sectors;
            float x = sinf(phi) * cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi) * sinf(theta);
            verts[vi++] = x; verts[vi++] = y; verts[vi++] = z; // pos
            verts[vi++] = x; verts[vi++] = y; verts[vi++] = z; // normal = pos (unit sphere)
        }
    }

    int ii = 0;
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < sectors; j++) {
            int a = i * (sectors + 1) + j;
            int b = a + sectors + 1;
            indices[ii++] = a; indices[ii++] = b; indices[ii++] = a + 1;
            indices[ii++] = a + 1; indices[ii++] = b; indices[ii++] = b + 1;
        }
    }

    glGenVertexArrays(1, &r->sphere_vao);
    glGenBuffers(1, &r->sphere_vbo);
    glGenBuffers(1, &r->sphere_ebo);
    glBindVertexArray(r->sphere_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->sphere_vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * 6 * sizeof(float), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->sphere_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    r->sphere_index_count = idx_count;

    free(verts);
    free(indices);
}

// Pyramid: 4 triangle faces + square base
static void gen_pyramid(Renderer *r) {
    // base corners at y=0, apex at y=1
    float v[] = {
        // front face
         0.0f, 1.0f, 0.0f,   0,0.4f,0.9f,
        -0.5f, 0.0f, 0.5f,   0,0.4f,0.9f,
         0.5f, 0.0f, 0.5f,   0,0.4f,0.9f,
        // right face
         0.0f, 1.0f, 0.0f,   0.9f,0.4f,0,
         0.5f, 0.0f, 0.5f,   0.9f,0.4f,0,
         0.5f, 0.0f,-0.5f,   0.9f,0.4f,0,
        // back face
         0.0f, 1.0f, 0.0f,   0,-0.4f,-0.9f,
         0.5f, 0.0f,-0.5f,   0,-0.4f,-0.9f,
        -0.5f, 0.0f,-0.5f,   0,-0.4f,-0.9f,
        // left face
         0.0f, 1.0f, 0.0f,  -0.9f,0.4f,0,
        -0.5f, 0.0f,-0.5f,  -0.9f,0.4f,0,
        -0.5f, 0.0f, 0.5f,  -0.9f,0.4f,0,
        // base 1
        -0.5f, 0.0f, 0.5f,   0,-1,0,
         0.5f, 0.0f, 0.5f,   0,-1,0,
         0.5f, 0.0f,-0.5f,   0,-1,0,
        // base 2
        -0.5f, 0.0f, 0.5f,   0,-1,0,
         0.5f, 0.0f,-0.5f,   0,-1,0,
        -0.5f, 0.0f,-0.5f,   0,-1,0,
    };

    glGenVertexArrays(1, &r->pyramid_vao);
    glGenBuffers(1, &r->pyramid_vbo);
    glBindVertexArray(r->pyramid_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->pyramid_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    r->pyramid_index_count = 18;
}

// Plane: simple quad
static void gen_plane(Renderer *r) {
    float v[] = {
        -0.5f, 0, -0.5f,  0,1,0,
         0.5f, 0, -0.5f,  0,1,0,
         0.5f, 0,  0.5f,  0,1,0,
        -0.5f, 0, -0.5f,  0,1,0,
         0.5f, 0,  0.5f,  0,1,0,
        -0.5f, 0,  0.5f,  0,1,0,
    };

    glGenVertexArrays(1, &r->plane_vao);
    glGenBuffers(1, &r->plane_vbo);
    glBindVertexArray(r->plane_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->plane_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    r->plane_index_count = 6;
}

// 2D quad (unit square centered at origin)
static void gen_quad_2d(Renderer *r) {
    float v[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f,
    };

    glGenVertexArrays(1, &r->quad_vao);
    glGenBuffers(1, &r->quad_vbo);
    glBindVertexArray(r->quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// 2D circle (fan of triangles)
static void gen_circle_2d(Renderer *r) {
    int segments = 64;
    int vert_count = segments + 2; // center + rim + close
    float *v = malloc(vert_count * 2 * sizeof(float));

    v[0] = 0; v[1] = 0; // center
    for (int i = 0; i <= segments; i++) {
        float a = 2.0f * M_PI * i / segments;
        v[(i + 1) * 2]     = cosf(a);
        v[(i + 1) * 2 + 1] = sinf(a);
    }

    glGenVertexArrays(1, &r->circle_vao);
    glGenBuffers(1, &r->circle_vbo);
    glBindVertexArray(r->circle_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->circle_vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * 2 * sizeof(float), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    r->circle_vert_count = vert_count;

    free(v);
}

// ---- Init ----

int renderer_init(Renderer *r) {
    memset(r, 0, sizeof(Renderer));

    r->shader_3d = link_program(vert_3d_src, frag_3d_src);
    r->shader_2d = link_program(vert_2d_src, frag_2d_src);

    if (!r->shader_3d || !r->shader_2d) {
        fprintf(stderr, "Failed to compile shaders\n");
        return 0;
    }

    gen_cube(r);
    gen_sphere(r);
    gen_pyramid(r);
    gen_plane(r);
    gen_quad_2d(r);
    gen_circle_2d(r);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return 1;
}

// ---- Uniform helpers ----

static void set_mat4(unsigned int prog, const char *name, Mat4 *m) {
    glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, m->m);
}

static void set_vec3(unsigned int prog, const char *name, float x, float y, float z) {
    glUniform3f(glGetUniformLocation(prog, name), x, y, z);
}

static void set_float(unsigned int prog, const char *name, float v) {
    glUniform1f(glGetUniformLocation(prog, name), v);
}

// ---- Build model matrix from shape props ----

static Mat4 build_model(ShapeProps *p) {
    Mat4 m = mat4_translate(p->pos.x, p->pos.y, p->pos.z);
    if (p->has_rotate) {
        m = mat4_mul(m, mat4_rotate_x(p->rotate.x));
        m = mat4_mul(m, mat4_rotate_y(p->rotate.y));
        m = mat4_mul(m, mat4_rotate_z(p->rotate.z));
    }
    m = mat4_mul(m, mat4_scale(p->scale.x, p->scale.y, p->scale.z));
    return m;
}

// ---- Draw 3D shape ----

static void draw_3d_mesh(Renderer *r, unsigned int vao, int count, ShapeProps *props, Mat4 extra_scale) {
    unsigned int s = r->shader_3d;
    glUseProgram(s);

    Mat4 model = build_model(props);
    model = mat4_mul(model, extra_scale);

    set_mat4(s, "uModel", &model);
    set_mat4(s, "uView", &r->view);
    set_mat4(s, "uProj", &r->projection_3d);
    set_vec3(s, "uColor", props->color.r, props->color.g, props->color.b);
    set_float(s, "uOpacity", props->opacity);

    if (props->fill == FILL_WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, count);

    if (props->fill == FILL_WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void draw_3d_mesh_indexed(Renderer *r, unsigned int vao, int count, ShapeProps *props, Mat4 extra_scale) {
    unsigned int s = r->shader_3d;
    glUseProgram(s);

    Mat4 model = build_model(props);
    model = mat4_mul(model, extra_scale);

    set_mat4(s, "uModel", &model);
    set_mat4(s, "uView", &r->view);
    set_mat4(s, "uProj", &r->projection_3d);
    set_vec3(s, "uColor", props->color.r, props->color.g, props->color.b);
    set_float(s, "uOpacity", props->opacity);

    if (props->fill == FILL_WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);

    if (props->fill == FILL_WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// ---- Draw 2D shape ----

static void draw_2d_rect(Renderer *r, ShapeProps *props, Vec2 size) {
    unsigned int s = r->shader_2d;
    glUseProgram(s);

    // 2D model: translate to pos, scale to size
    Mat4 model = mat4_translate(props->pos.x + size.x / 2, props->pos.y + size.y / 2, 0);
    if (props->has_rotate)
        model = mat4_mul(model, mat4_rotate_z(props->rotate.z));
    model = mat4_mul(model, mat4_scale(size.x, size.y, 1));

    set_mat4(s, "uProj", &r->projection_2d);
    set_mat4(s, "uModel", &model);
    set_vec3(s, "uColor", props->color.r, props->color.g, props->color.b);
    set_float(s, "uOpacity", props->opacity);

    glBindVertexArray(r->quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void draw_2d_circle(Renderer *r, ShapeProps *props, float radius) {
    unsigned int s = r->shader_2d;
    glUseProgram(s);

    Mat4 model = mat4_translate(props->pos.x, props->pos.y, 0);
    model = mat4_mul(model, mat4_scale(radius, radius, 1));

    set_mat4(s, "uProj", &r->projection_2d);
    set_mat4(s, "uModel", &model);
    set_vec3(s, "uColor", props->color.r, props->color.g, props->color.b);
    set_float(s, "uOpacity", props->opacity);

    glBindVertexArray(r->circle_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, r->circle_vert_count);
}

static void draw_2d_triangle(Renderer *r, ShapeProps *props, Vec2 p1, Vec2 p2, Vec2 p3) {
    unsigned int s = r->shader_2d;
    glUseProgram(s);

    float verts[] = { p1.x, p1.y, p2.x, p2.y, p3.x, p3.y };

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Mat4 model = mat4_identity();
    set_mat4(s, "uProj", &r->projection_2d);
    set_mat4(s, "uModel", &model);
    set_vec3(s, "uColor", props->color.r, props->color.g, props->color.b);
    set_float(s, "uOpacity", props->opacity);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

static void draw_2d_line(Renderer *r, ShapeProps *props, Vec2 from, Vec2 to, float width) {
    unsigned int s = r->shader_2d;
    glUseProgram(s);

    float verts[] = { from.x, from.y, to.x, to.y };

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Mat4 model = mat4_identity();
    set_mat4(s, "uProj", &r->projection_2d);
    set_mat4(s, "uModel", &model);
    set_vec3(s, "uColor", props->color.r, props->color.g, props->color.b);
    set_float(s, "uOpacity", props->opacity);

    glLineWidth(width);
    glDrawArrays(GL_LINES, 0, 2);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

// ---- Draw a single node ----

static void draw_node(Renderer *r, ASTNode *node) {
    switch (node->type) {
        case NODE_CIRCLE:
            draw_2d_circle(r, &node->data.circle.props, node->data.circle.radius);
            break;
        case NODE_RECT:
            draw_2d_rect(r, &node->data.rect.props, node->data.rect.size);
            break;
        case NODE_LINE:
            draw_2d_line(r, &node->data.line.props,
                         node->data.line.from, node->data.line.to,
                         node->data.line.width);
            break;
        case NODE_TRIANGLE:
            draw_2d_triangle(r, &node->data.triangle.props,
                             node->data.triangle.p1, node->data.triangle.p2,
                             node->data.triangle.p3);
            break;
        case NODE_CUBE: {
            Mat4 s = mat4_scale(node->data.cube.size.x, node->data.cube.size.y, node->data.cube.size.z);
            draw_3d_mesh(r, r->cube_vao, r->cube_index_count, &node->data.cube.props, s);
            break;
        }
        case NODE_SPHERE: {
            float rad = node->data.sphere.radius;
            Mat4 s = mat4_scale(rad, rad, rad);
            draw_3d_mesh_indexed(r, r->sphere_vao, r->sphere_index_count, &node->data.sphere.props, s);
            break;
        }
        case NODE_PYRAMID: {
            Mat4 s = mat4_scale(node->data.pyramid.base, node->data.pyramid.height, node->data.pyramid.base);
            draw_3d_mesh(r, r->pyramid_vao, r->pyramid_index_count, &node->data.pyramid.props, s);
            break;
        }
        case NODE_PLANE: {
            Mat4 s = mat4_scale(node->data.plane.size.x, 1, node->data.plane.size.y);
            draw_3d_mesh(r, r->plane_vao, r->plane_index_count, &node->data.plane.props, s);
            break;
        }
        case NODE_GROUP: {
            // TODO: apply group transform as parent transform
            for (int i = 0; i < node->data.group.child_count; i++) {
                // offset children by group pos
                ASTNode *child = node->data.group.children[i];
                // quick hack: add group pos to child pos
                ShapeProps *cp = NULL;
                switch (child->type) {
                    case NODE_CIRCLE:   cp = &child->data.circle.props; break;
                    case NODE_RECT:     cp = &child->data.rect.props; break;
                    case NODE_CUBE:     cp = &child->data.cube.props; break;
                    case NODE_SPHERE:   cp = &child->data.sphere.props; break;
                    case NODE_PYRAMID:  cp = &child->data.pyramid.props; break;
                    case NODE_PLANE:    cp = &child->data.plane.props; break;
                    case NODE_CYLINDER: cp = &child->data.cylinder.props; break;
                    default: break;
                }
                Vec3 orig = {0};
                if (cp) {
                    orig = cp->pos;
                    cp->pos.x += node->data.group.props.pos.x;
                    cp->pos.y += node->data.group.props.pos.y;
                    cp->pos.z += node->data.group.props.pos.z;
                }
                draw_node(r, child);
                if (cp) cp->pos = orig; // restore
            }
            break;
        }
        case NODE_TEXT:
            // text rendering requires font atlas — skip for now
            break;
        case NODE_CYLINDER:
            // TODO: generate cylinder mesh
            break;
        case NODE_LIGHT:
        case NODE_SCENE:
            break;
    }
}

// ---- Draw entire scene ----

void renderer_draw_scene(Renderer *r, Scene *scene) {
    SceneSettings *s = &scene->settings;

    glClearColor(s->background.r, s->background.g, s->background.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup 3D matrices
    float aspect = (float)s->width / (float)s->height;
    r->projection_3d = mat4_perspective(45.0f, aspect, 0.1f, 100.0f);

    if (s->has_camera) {
        r->view = mat4_lookat(s->camera.x, s->camera.y, s->camera.z,
                              0, 0, 0,
                              0, 1, 0);
    } else {
        r->view = mat4_lookat(0, 2, -8, 0, 0, 0, 0, 1, 0);
    }

    // Setup 2D matrices
    r->projection_2d = mat4_ortho(0, (float)s->width, (float)s->height, 0, -1, 1);

    // Find lights and set uniforms
    Vec3 light_dir = {0.5f, 1.0f, 0.5f};
    Vec3 light_color = {1, 1, 1};
    float ambient = 0.2f;

    for (int i = 0; i < scene->node_count; i++) {
        if (scene->nodes[i]->type == NODE_LIGHT) {
            LightNode *l = &scene->nodes[i]->data.light;
            if (l->type == LIGHT_DIRECTIONAL || l->type == LIGHT_POINT) {
                light_dir = l->pos;
                light_color = (Vec3){l->color.r, l->color.g, l->color.b};
            }
            if (l->type == LIGHT_AMBIENT) {
                ambient = l->intensity;
            }
        }
    }

    // Set light uniforms on 3D shader
    glUseProgram(r->shader_3d);
    set_vec3(r->shader_3d, "uLightDir", light_dir.x, light_dir.y, light_dir.z);
    set_vec3(r->shader_3d, "uLightColor", light_color.x, light_color.y, light_color.z);
    set_float(r->shader_3d, "uAmbient", ambient);

    // Draw 3D shapes first (with depth testing)
    glEnable(GL_DEPTH_TEST);
    for (int i = 0; i < scene->node_count; i++) {
        NodeType t = scene->nodes[i]->type;
        if (t == NODE_CUBE || t == NODE_SPHERE || t == NODE_PYRAMID ||
            t == NODE_PLANE || t == NODE_CYLINDER || t == NODE_GROUP) {
            draw_node(r, scene->nodes[i]);
        }
    }

    // Draw 2D shapes on top (no depth testing)
    glDisable(GL_DEPTH_TEST);
    for (int i = 0; i < scene->node_count; i++) {
        NodeType t = scene->nodes[i]->type;
        if (t == NODE_CIRCLE || t == NODE_RECT || t == NODE_LINE ||
            t == NODE_TRIANGLE || t == NODE_TEXT) {
            draw_node(r, scene->nodes[i]);
        }
    }
}

void renderer_destroy(Renderer *r) {
    glDeleteProgram(r->shader_3d);
    glDeleteProgram(r->shader_2d);
    glDeleteVertexArrays(1, &r->cube_vao);
    glDeleteBuffers(1, &r->cube_vbo);
    glDeleteVertexArrays(1, &r->sphere_vao);
    glDeleteBuffers(1, &r->sphere_vbo);
    glDeleteBuffers(1, &r->sphere_ebo);
    glDeleteVertexArrays(1, &r->pyramid_vao);
    glDeleteBuffers(1, &r->pyramid_vbo);
    glDeleteVertexArrays(1, &r->plane_vao);
    glDeleteBuffers(1, &r->plane_vbo);
    glDeleteVertexArrays(1, &r->quad_vao);
    glDeleteBuffers(1, &r->quad_vbo);
    glDeleteVertexArrays(1, &r->circle_vao);
    glDeleteBuffers(1, &r->circle_vbo);
}
