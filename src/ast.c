#include "ast.h"
#include <stdlib.h>
#include <string.h>

Scene *scene_create(void) {
    Scene *scene = calloc(1, sizeof(Scene));
    scene->settings.width = 800;
    scene->settings.height = 600;
    strcpy(scene->settings.title, "Prismo");
    scene->settings.background = (Color){0.05f, 0.05f, 0.1f, 1.0f};
    scene->settings.camera = (Vec3){0, 0, -10};
    scene->settings.has_camera = 0;
    scene->node_capacity = 16;
    scene->nodes = calloc(scene->node_capacity, sizeof(ASTNode *));
    return scene;
}

void scene_add_node(Scene *scene, ASTNode *node) {
    if (scene->node_count >= scene->node_capacity) {
        scene->node_capacity *= 2;
        scene->nodes = realloc(scene->nodes, scene->node_capacity * sizeof(ASTNode *));
    }
    scene->nodes[scene->node_count++] = node;
}

void group_add_child(GroupNode *group, ASTNode *node) {
    if (group->child_count >= group->child_capacity) {
        group->child_capacity = group->child_capacity == 0 ? 8 : group->child_capacity * 2;
        group->children = realloc(group->children, group->child_capacity * sizeof(ASTNode *));
    }
    group->children[group->child_count++] = node;
}

static void node_free(ASTNode *node) {
    if (!node) return;
    if (node->type == NODE_GROUP) {
        for (int i = 0; i < node->data.group.child_count; i++) {
            node_free(node->data.group.children[i]);
        }
        free(node->data.group.children);
    }
    free(node);
}

void scene_free(Scene *scene) {
    if (!scene) return;
    for (int i = 0; i < scene->node_count; i++) {
        node_free(scene->nodes[i]);
    }
    free(scene->nodes);
    free(scene);
}
