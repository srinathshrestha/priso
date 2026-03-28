#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Nuklear implementation (must be in exactly one .c file)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear_glfw_gl3.h"

#include "editor.h"
#include "parser.h"
#include "lexer.h"

// ---- Syntax highlighting colors ----

static struct nk_color syntax_color(TokenType type) {
    switch (type) {
        // Scene keywords — blue
        case TOK_CANVAS: case TOK_BACKGROUND: case TOK_CAMERA:
        case TOK_LIGHT: case TOK_GROUP:
            return nk_rgb(100, 160, 255);

        // Shape keywords — purple
        case TOK_CIRCLE: case TOK_RECT: case TOK_LINE:
        case TOK_TRIANGLE: case TOK_TEXT:
        case TOK_CUBE: case TOK_SPHERE: case TOK_PYRAMID:
        case TOK_PLANE: case TOK_CYLINDER:
            return nk_rgb(190, 130, 255);

        // Property keywords — cyan
        case TOK_POS: case TOK_RADIUS: case TOK_SIZE:
        case TOK_COLOR: case TOK_ROTATE: case TOK_SCALE:
        case TOK_OPACITY: case TOK_FILL: case TOK_FROM:
        case TOK_TO: case TOK_WIDTH: case TOK_HEIGHT:
        case TOK_BASE: case TOK_CONTENT: case TOK_TYPE:
        case TOK_INTENSITY: case TOK_P1: case TOK_P2: case TOK_P3:
            return nk_rgb(80, 220, 200);

        // Enum values — yellow
        case TOK_SOLID: case TOK_WIREFRAME:
        case TOK_POINT: case TOK_DIRECTIONAL: case TOK_AMBIENT:
            return nk_rgb(230, 200, 80);

        // Numbers — green
        case TOK_NUMBER:
            return nk_rgb(130, 220, 100);

        // Strings — orange
        case TOK_STRING:
            return nk_rgb(240, 170, 80);

        // Hex colors — pink
        case TOK_HEX_COLOR:
            return nk_rgb(240, 120, 180);

        // Braces — white
        case TOK_LBRACE: case TOK_RBRACE:
            return nk_rgb(200, 200, 200);

        // Error — red
        case TOK_ERROR:
            return nk_rgb(255, 80, 80);

        default:
            return nk_rgb(180, 180, 180);
    }
}

// Draw syntax-highlighted text over the edit widget
static void draw_syntax_highlight(struct nk_context *ctx, Editor *e,
                                   struct nk_rect edit_bounds, const struct nk_user_font *font) {
    struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
    if (!canvas || !font || e->text_len == 0) return;

    // Use actual Nuklear edit style metrics for alignment
    float char_w = font->width(font->userdata, font->height, "M", 1);
    float line_h = font->height + ctx->style.edit.row_padding;
    float pad_x = ctx->style.edit.padding.x;
    float pad_y = ctx->style.edit.padding.y;

    // First pass: build a color map for each character (comment-aware)
    // Static buffer avoids per-frame malloc (max 64K chars * 4 bytes = 256KB)
    static struct nk_color char_colors[EDITOR_MAX_TEXT];
    if (e->text_len > EDITOR_MAX_TEXT) return;

    struct nk_color comment_color = nk_rgb(100, 110, 120);
    struct nk_color default_color = nk_rgb(180, 180, 180);

    // Init all to default
    for (int i = 0; i < e->text_len; i++) {
        char_colors[i] = default_color;
    }

    // Mark comments (// and # style)
    {
        int i = 0;
        while (i < e->text_len) {
            char c = e->text[i];
            if (c == '/' && i + 1 < e->text_len && e->text[i + 1] == '/') {
                while (i < e->text_len && e->text[i] != '\n') {
                    char_colors[i] = comment_color;
                    i++;
                }
            } else if (c == '#' && (i + 1 >= e->text_len || !isxdigit((unsigned char)e->text[i + 1]))) {
                while (i < e->text_len && e->text[i] != '\n') {
                    char_colors[i] = comment_color;
                    i++;
                }
            } else {
                i++;
            }
        }
    }

    // Tokenize and color non-comment regions
    {
        Lexer lex;
        lexer_init(&lex, e->text);
        for (;;) {
            Token tok = lexer_next(&lex);
            if (tok.type == TOK_EOF) break;

            int start = (int)(tok.start - e->text);
            struct nk_color col = syntax_color(tok.type);

            for (int j = 0; j < tok.length && (start + j) < e->text_len; j++) {
                // Don't overwrite comment colors
                if (char_colors[start + j].r != comment_color.r ||
                    char_colors[start + j].g != comment_color.g ||
                    char_colors[start + j].b != comment_color.b) {
                    char_colors[start + j] = col;
                }
            }

            if (tok.type == TOK_ERROR) break;
        }
    }

    // Draw each character with its color
    int line = 0, col = 0;
    struct nk_color bg_transparent = nk_rgba(0, 0, 0, 0);
    char ch_buf[2] = {0, 0};

    for (int i = 0; i < e->text_len; i++) {
        if (e->text[i] == '\n') {
            line++;
            col = 0;
            continue;
        }

        float x = edit_bounds.x + pad_x + col * char_w;
        float y = edit_bounds.y + pad_y + line * line_h;

        // Only draw if within the edit bounds (clipped by Nuklear)
        if (y + line_h >= edit_bounds.y && y <= edit_bounds.y + edit_bounds.h &&
            x + char_w >= edit_bounds.x && x <= edit_bounds.x + edit_bounds.w) {
            ch_buf[0] = e->text[i];
            struct nk_rect ch_rect = nk_rect(x, y, char_w, line_h);
            nk_draw_text(canvas, ch_rect, ch_buf, 1, font, bg_transparent, char_colors[i]);
        }

        col++;
    }

}

// ---- Editor API ----

void editor_init(Editor *e, const char *initial_code) {
    memset(e, 0, sizeof(Editor));
    e->pixel_ratio = 1.0f;

    if (initial_code) {
        e->text_len = (int)strlen(initial_code);
        if (e->text_len >= EDITOR_MAX_TEXT) e->text_len = EDITOR_MAX_TEXT - 1;
        memcpy(e->text, initial_code, e->text_len);
        e->text[e->text_len] = '\0';
    }

    if (renderer_init(&e->renderer)) {
        e->renderer_ready = 1;
    } else {
        fprintf(stderr, "Failed to init renderer for editor\n");
    }
}

void editor_destroy(Editor *e) {
    if (e->scene) {
        scene_free(e->scene);
        e->scene = NULL;
    }
    if (e->renderer_ready) {
        renderer_destroy(&e->renderer);
    }
}

void editor_run(Editor *e) {
    if (e->scene) {
        scene_free(e->scene);
        e->scene = NULL;
    }
    e->has_error = 0;
    e->error_msg[0] = '\0';

    e->text[e->text_len] = '\0';

    int had_error = 0;
    char err_msg[512] = {0};
    Scene *scene = parse_with_error(e->text, &had_error, err_msg, sizeof(err_msg));

    if (had_error) {
        e->has_error = 1;
        snprintf(e->error_msg, sizeof(e->error_msg), "%s", err_msg);
    }

    e->scene = scene;
}

// Render scene directly to the screen in the viewport region.
// Called AFTER Nuklear has rendered, so it draws on top of the empty viewport panel.
void editor_render_scene_direct(Editor *e, int window_h) {
    if (!e->scene || !e->renderer_ready) return;

    float pr = e->pixel_ratio > 0 ? e->pixel_ratio : 1.0f;

    // Convert logical viewport coords to pixel coords (for Retina)
    int px = (int)(e->vp_x * pr);
    int py = (int)((window_h - e->vp_y - e->vp_h) * pr); // flip Y: OpenGL origin is bottom-left
    int pw = (int)(e->vp_w * pr);
    int ph = (int)(e->vp_h * pr);

    if (pw <= 0 || ph <= 0) return;

    // Update spin
    if (e->spinning) {
        e->spin_angle += 0.5f;
        if (e->spin_angle >= 360.0f) e->spin_angle -= 360.0f;

        for (int i = 0; i < e->scene->node_count; i++) {
            ASTNode *node = e->scene->nodes[i];
            ShapeProps *props = NULL;
            switch (node->type) {
                case NODE_CUBE:     props = &node->data.cube.props; break;
                case NODE_SPHERE:   props = &node->data.sphere.props; break;
                case NODE_PYRAMID:  props = &node->data.pyramid.props; break;
                case NODE_PLANE:    props = &node->data.plane.props; break;
                case NODE_CYLINDER: props = &node->data.cylinder.props; break;
                default: break;
            }
            if (props) {
                props->rotate.y = e->spin_angle;
                props->has_rotate = 1;
            }
        }
    }

    // Set scene dimensions to match the viewport pixel size
    e->scene->settings.width = pw;
    e->scene->settings.height = ph;

    // Restrict rendering to the viewport area
    glViewport(px, py, pw, ph);
    glEnable(GL_SCISSOR_TEST);
    glScissor(px, py, pw, ph);

    // Render the scene (glClear will only affect scissor region)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderer_draw_scene(&e->renderer, e->scene);

    // Restore state
    glDisable(GL_SCISSOR_TEST);
}

void editor_ui(Editor *e, struct nk_context *ctx, int win_w, int win_h) {
    float left_w = (float)win_w * 0.45f;
    float right_w = (float)win_w - left_w;

    struct nk_color btn_color = nk_rgb(70, 130, 230);
    struct nk_color btn_hover = nk_rgb(90, 150, 250);
    struct nk_color err_color = nk_rgb(255, 80, 80);
    struct nk_color ok_color = nk_rgb(80, 200, 120);

    // ---- Left Panel: Code Editor ----
    if (nk_begin(ctx, "Editor",
            nk_rect(0, 0, left_w, (float)win_h),
            NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {

        // Toolbar row — explicit widths so text fits
        float toolbar_widths[] = {80, 50, 80};
        nk_layout_row(ctx, NK_STATIC, 30, 3, toolbar_widths);

        struct nk_style_button run_style = ctx->style.button;
        run_style.normal = nk_style_item_color(btn_color);
        run_style.hover = nk_style_item_color(btn_hover);
        run_style.text_normal = nk_rgb(255, 255, 255);
        run_style.text_hover = nk_rgb(255, 255, 255);
        run_style.rounding = 4;

        if (nk_button_label_styled(ctx, &run_style, "Run")) {
            editor_run(e);
        }

        if (e->has_error) {
            nk_label_colored(ctx, "Error", NK_TEXT_CENTERED, err_color);
        } else if (e->scene) {
            nk_label_colored(ctx, "OK", NK_TEXT_CENTERED, ok_color);
        } else {
            nk_label(ctx, "Ready", NK_TEXT_CENTERED);
        }

        if (e->scene) {
            char info[64];
            snprintf(info, sizeof(info), "%d nodes", e->scene->node_count);
            nk_label(ctx, info, NK_TEXT_RIGHT);
        } else {
            nk_label(ctx, "", NK_TEXT_RIGHT);
        }

        if (e->has_error && e->error_msg[0]) {
            nk_layout_row_dynamic(ctx, 18, 1);
            nk_label_colored(ctx, e->error_msg, NK_TEXT_LEFT, err_color);
        }

        // Code editor — use remaining height
        struct nk_rect content = nk_window_get_content_region(ctx);
        float used = 42;
        if (e->has_error && e->error_msg[0]) used += 22;
        float editor_h = content.h - used;
        if (editor_h < 100) editor_h = 100;

        // Calculate content size for scrollable area
        int line_count = 1, max_line_len = 0, cur_line_len = 0;
        for (int i = 0; i < e->text_len; i++) {
            if (e->text[i] == '\n') {
                line_count++;
                if (cur_line_len > max_line_len) max_line_len = cur_line_len;
                cur_line_len = 0;
            } else {
                cur_line_len++;
            }
        }
        if (cur_line_len > max_line_len) max_line_len = cur_line_len;

        // Make edit widget wide enough that text doesn't wrap
        const struct nk_user_font *font = ctx->style.font;
        float char_w = font->width(font->userdata, font->height, "M", 1);
        float line_h = font->height + ctx->style.edit.row_padding;
        float edit_w = (max_line_len + 10) * char_w;
        float min_w = left_w - 40;
        if (edit_w < min_w) edit_w = min_w;
        float edit_h = (line_count + 3) * line_h;
        if (edit_h < editor_h - 20) edit_h = editor_h - 20;

        // Scrollable group provides horizontal + vertical scrollbars
        nk_layout_row_dynamic(ctx, editor_h, 1);
        if (nk_group_begin(ctx, "code_scroll", 0)) {
            nk_layout_row_static(ctx, edit_h, (int)edit_w, 1);

            // Make edit text invisible — syntax overlay will draw colored text
            struct nk_color orig_text   = ctx->style.edit.text_normal;
            struct nk_color orig_active = ctx->style.edit.text_active;
            struct nk_color orig_hover  = ctx->style.edit.text_hover;
            struct nk_color orig_sel    = ctx->style.edit.selected_text_normal;
            struct nk_color edit_bg     = ctx->style.edit.active.data.color;
            ctx->style.edit.text_normal          = edit_bg;
            ctx->style.edit.text_active          = edit_bg;
            ctx->style.edit.text_hover           = edit_bg;
            ctx->style.edit.selected_text_normal = edit_bg;

            // Capture bounds before the widget consumes the layout slot
            struct nk_rect edit_bounds = nk_widget_bounds(ctx);

            nk_edit_string(ctx,
                NK_EDIT_BOX | NK_EDIT_MULTILINE | NK_EDIT_CLIPBOARD,
                e->text, &e->text_len, EDITOR_MAX_TEXT,
                nk_filter_default);

            // Draw syntax-colored text overlay (while still inside group for correct clipping)
            draw_syntax_highlight(ctx, e, edit_bounds, ctx->style.font);

            // Restore original text colors
            ctx->style.edit.text_normal          = orig_text;
            ctx->style.edit.text_active          = orig_active;
            ctx->style.edit.text_hover           = orig_hover;
            ctx->style.edit.selected_text_normal = orig_sel;

            nk_group_end(ctx);
        }
    }
    nk_end(ctx);

    // ---- Right Panel: Viewport ----
    if (nk_begin(ctx, "Viewport",
            nk_rect(left_w, 0, right_w, (float)win_h),
            NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE)) {

        // Spin toggle button row — push to right side
        float spin_widths[] = {right_w - 130, 100};
        nk_layout_row(ctx, NK_STATIC, 28, 2, spin_widths);
        nk_label(ctx, "", NK_TEXT_LEFT);
        if (e->scene) {
            struct nk_style_button spin_style = ctx->style.button;
            if (e->spinning) {
                spin_style.normal = nk_style_item_color(nk_rgb(200, 80, 80));
                spin_style.hover = nk_style_item_color(nk_rgb(220, 100, 100));
            } else {
                spin_style.normal = nk_style_item_color(nk_rgb(70, 180, 130));
                spin_style.hover = nk_style_item_color(nk_rgb(90, 200, 150));
            }
            spin_style.text_normal = nk_rgb(255, 255, 255);
            spin_style.text_hover = nk_rgb(255, 255, 255);
            spin_style.rounding = 4;
            if (nk_button_label_styled(ctx, &spin_style, e->spinning ? "Stop Spin" : "Spin")) {
                e->spinning = !e->spinning;
                if (!e->spinning) {
                    e->spin_angle = 0;
                    editor_run(e);
                }
            }
        } else {
            nk_label(ctx, "", NK_TEXT_LEFT);
        }

        // Use a 1px spacer to get the actual Y position after the spin bar
        nk_layout_row_dynamic(ctx, 1, 1);
        // Get the bounds of this spacer — its Y position is where the scene area starts
        struct nk_rect spacer_bounds = nk_widget_bounds(ctx);
        nk_label(ctx, "", NK_TEXT_LEFT);

        // Calculate viewport rect from actual Nuklear layout positions
        struct nk_rect panel_bounds = nk_window_get_bounds(ctx);
        float pad = 4.0f;
        e->vp_x = panel_bounds.x + pad;
        e->vp_y = spacer_bounds.y + spacer_bounds.h;
        e->vp_w = panel_bounds.w - pad * 2;
        e->vp_h = (panel_bounds.y + panel_bounds.h) - e->vp_y - pad;
        if (e->vp_w < 10) e->vp_w = 10;
        if (e->vp_h < 10) e->vp_h = 10;

        if (!e->scene) {
            struct nk_rect content = nk_window_get_content_region(ctx);
            float remaining = content.h - (spacer_bounds.y - content.y) - 40;
            if (remaining > 60) {
                nk_layout_row_dynamic(ctx, remaining / 2 - 20, 1);
                nk_label(ctx, "", NK_TEXT_LEFT);
            }
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_label(ctx, "Write some Prismo code and press Run!", NK_TEXT_CENTERED);
        }
    }
    nk_end(ctx);
}
