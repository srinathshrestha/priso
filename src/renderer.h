#ifndef RENDERER_H
#define RENDERER_H

#include "ast.h"
#include "math_util.h"

typedef struct {
    unsigned int shader_3d;
    unsigned int shader_2d;

    // 3D mesh VAOs/VBOs
    unsigned int cube_vao, cube_vbo, cube_ebo;
    int cube_index_count;

    unsigned int sphere_vao, sphere_vbo, sphere_ebo;
    int sphere_index_count;

    unsigned int pyramid_vao, pyramid_vbo, pyramid_ebo;
    int pyramid_index_count;

    unsigned int plane_vao, plane_vbo, plane_ebo;
    int plane_index_count;

    unsigned int cylinder_vao, cylinder_vbo, cylinder_ebo;
    int cylinder_index_count;

    // 2D
    unsigned int quad_vao, quad_vbo;
    unsigned int circle_vao, circle_vbo;
    int circle_vert_count;

    // matrices
    Mat4 projection_3d;
    Mat4 view;
    Mat4 projection_2d;
} Renderer;

int  renderer_init(Renderer *r);
void renderer_draw_scene(Renderer *r, Scene *scene);
void renderer_destroy(Renderer *r);

#endif
