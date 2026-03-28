#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#endif
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "renderer.h"
#include "editor.h"

// Nuklear headers (no implementation — that's in editor.c)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#define MAX_VERTEX_BUFFER  (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)

// ---- File loading ----

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, size, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

// ---- GLFW callbacks (CLI mode only) ----

static void error_callback(int error, const char *desc) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, desc);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // R to reload
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        int *reload = (int *)glfwGetWindowUserPointer(window);
        if (reload) *reload = 1;
    }
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

// ---- CLI mode (original behavior) ----

static int run_cli(const char *filepath) {
    char *source = read_file(filepath);
    if (!source) return 1;

    Scene *scene = parse(source);
    if (!scene) {
        fprintf(stderr, "Failed to parse scene\n");
        free(source);
        return 1;
    }

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        scene_free(scene);
        free(source);
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(
        scene->settings.width, scene->settings.height,
        scene->settings.title, NULL, NULL);

    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        scene_free(scene);
        free(source);
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int reload_flag = 0;
    glfwSetWindowUserPointer(window, &reload_flag);

    glEnable(GL_MULTISAMPLE);

    printf("Prismo renderer\n");
    printf("  OpenGL: %s\n", glGetString(GL_VERSION));
    printf("  File:   %s\n", filepath);
    printf("  Canvas: %dx%d\n", scene->settings.width, scene->settings.height);
    printf("\nControls:\n");
    printf("  ESC  - quit\n");
    printf("  R    - reload scene file\n");

    Renderer renderer;
    if (!renderer_init(&renderer)) {
        fprintf(stderr, "Failed to init renderer\n");
        scene_free(scene);
        free(source);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        if (reload_flag) {
            reload_flag = 0;
            printf("\nReloading %s...\n", filepath);
            free(source);
            scene_free(scene);

            source = read_file(filepath);
            if (source) {
                Scene *new_scene = parse(source);
                if (new_scene) {
                    scene = new_scene;
                    glfwSetWindowSize(window, scene->settings.width, scene->settings.height);
                    glfwSetWindowTitle(window, scene->settings.title);
                } else {
                    fprintf(stderr, "Parse failed, keeping previous scene\n");
                }
            } else {
                fprintf(stderr, "Failed to read file, keeping previous scene\n");
            }
        }

        if (!scene) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        int fb_w, fb_h;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);

        int win_w, win_h;
        glfwGetWindowSize(window, &win_w, &win_h);
        scene->settings.width = win_w;
        scene->settings.height = win_h;

        renderer_draw_scene(&renderer, scene);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer_destroy(&renderer);
    scene_free(scene);
    free(source);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// ---- Editor mode ----

static const char *default_code =
    "canvas 800 600 \"My Scene\"\n"
    "background #1a1a2e\n"
    "camera 0 3 -8\n"
    "\n"
    "light { type directional  pos 5 10 5  color #ffffff  intensity 1.0 }\n"
    "light { type ambient  color #333333  intensity 0.3 }\n"
    "\n"
    "// Add your shapes here!\n"
    "sphere { pos 0 1 0  radius 1.5  color #e94560 }\n"
    "cube { pos -3 0.5 0  size 1  color #0f3460  rotate 0 45 0 }\n"
    "plane { pos 0 0 0  size 10 10  color #16213e }\n";

static int run_editor(void) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(1400, 900, "Prismo Editor", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glEnable(GL_MULTISAMPLE);

    printf("Prismo Editor\n");
    printf("  OpenGL: %s\n", glGetString(GL_VERSION));
    printf("\nControls:\n");
    printf("  Cmd+Enter - run scene\n");
    printf("  ESC       - quit\n");

    // Get content scale for Retina displays
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    // Init Nuklear (zero-init to avoid garbage in key_events etc.)
    struct nk_glfw glfw_nk;
    memset(&glfw_nk, 0, sizeof(glfw_nk));
    struct nk_context *ctx = nk_glfw3_init(&glfw_nk, window, NK_GLFW3_INSTALL_CALLBACKS);

    // Setup crisp font for Retina — load system monospace TTF
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&glfw_nk, &atlas);
    struct nk_font *font = nk_font_atlas_add_from_file(
        atlas, "/System/Library/Fonts/Menlo.ttc", 15.0f * yscale, NULL);
    if (!font) {
        // Fallback to default if Menlo not found
        font = nk_font_atlas_add_default(atlas, 15.0f * yscale, NULL);
    }
    nk_glfw3_font_stash_end(&glfw_nk);
    nk_style_set_font(ctx, &font->handle);

    // Init editor (static to keep 64KB+ buffer off the stack)
    static Editor editor;
    editor_init(&editor, default_code);
    editor.pixel_ratio = yscale;

    // Track Cmd+Enter for run shortcut
    int cmd_enter_was_down = 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int win_w, win_h;
        glfwGetWindowSize(window, &win_w, &win_h);

        // Check Cmd+Enter (macOS: Super key)
        int cmd_held = glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
                       glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
        int enter_held = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
                         glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
        int cmd_enter_down = cmd_held && enter_held;

        if (cmd_enter_down && !cmd_enter_was_down) {
            editor_run(&editor);
        }
        cmd_enter_was_down = cmd_enter_down;

        // Check ESC to quit
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // Nuklear frame: build UI commands
        nk_glfw3_new_frame(&glfw_nk);
        editor_ui(&editor, ctx, win_w, win_h);

        // Render Nuklear to screen first (UI background)
        int fb_w, fb_h;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nk_glfw3_render(&glfw_nk, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

        // Render scene directly on top of the viewport panel area
        editor_render_scene_direct(&editor, win_h);

        glfwSwapBuffers(window);
    }

    // Cleanup
    editor_destroy(&editor);
    nk_glfw3_shutdown(&glfw_nk);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// ---- Main ----

int main(int argc, char **argv) {
    if (argc >= 2) {
        return run_cli(argv[1]);
    }
    return run_editor();
}
