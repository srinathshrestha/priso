#ifndef EDITOR_H
#define EDITOR_H

#include "ast.h"
#include "renderer.h"

#define EDITOR_MAX_TEXT (64 * 1024)

typedef struct {
    // Code buffer
    char text[EDITOR_MAX_TEXT];
    int  text_len;

    // Scene state
    Scene *scene;
    Renderer renderer;
    int renderer_ready;

    // Error state
    int  has_error;
    char error_msg[512];

    // Display scale (Retina = 2.0)
    float pixel_ratio;

    // Viewport rect (in logical pixels, set by editor_ui)
    float vp_x, vp_y, vp_w, vp_h;

    // Spin animation
    int  spinning;
    float spin_angle;
} Editor;

// Forward declare nk_context to avoid including nuklear.h in the header
struct nk_context;

void editor_init(Editor *e, const char *initial_code);
void editor_destroy(Editor *e);
void editor_run(Editor *e);
void editor_render_scene_direct(Editor *e, int window_h);
void editor_ui(Editor *e, struct nk_context *ctx, int win_w, int win_h);

#endif
