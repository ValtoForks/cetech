#include <stdio.h>

#include <cetech/entity/entity.h>
#include <cetech/renderer/renderer.h>
#include <cetech/renderer/texture.h>
#include <cetech/debugui/debugui.h>
#include <cetech/level/level.h>
#include <cetech/camera/camera.h>
#include <cetech/playground/level_editor.h>
#include <cetech/transform/transform.h>
#include <cetech/input/input.h>
#include <cetech/renderer/viewport.h>
#include <cetech/playground/asset_browser.h>
#include <cetech/playground/explorer.h>
#include <cetech/debugui/private/ocornut-imgui/imgui.h>
#include <cetech/application/application.h>
#include <celib/fpumath.h>
#include <cetech/playground/playground.h>

#include "cetech/hashlib/hashlib.h"
#include "cetech/os/memory.h"
#include "cetech/api/api_system.h"
#include "cetech/module/module.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_renderer_a0);
CETECH_DECL_API(ct_hash_a0);
CETECH_DECL_API(ct_debugui_a0);
CETECH_DECL_API(ct_app_a0);
CETECH_DECL_API(ct_world_a0);
CETECH_DECL_API(ct_level_a0);
CETECH_DECL_API(ct_entity_a0);
CETECH_DECL_API(ct_transform_a0);
CETECH_DECL_API(ct_keyboard_a0);
CETECH_DECL_API(ct_camera_a0);
CETECH_DECL_API(ct_viewport_a0);
CETECH_DECL_API(ct_asset_browser_a0);
CETECH_DECL_API(ct_explorer_a0);
CETECH_DECL_API(ct_playground_a0);
CETECH_DECL_API(ct_ydb_a0);

using namespace celib;

#define MAX_EDITOR 8

static struct globals {
    bool visible[MAX_EDITOR];
    struct ct_viewport viewport[MAX_EDITOR];
    struct ct_world world[MAX_EDITOR];
    struct ct_entity camera_ent[MAX_EDITOR];
    struct ct_entity entity[MAX_EDITOR];
    const char *path[MAX_EDITOR];
    uint64_t root[MAX_EDITOR];
    uint64_t entity_name[MAX_EDITOR];
    bool is_first[MAX_EDITOR];
    bool is_level[MAX_EDITOR];

    uint8_t active_editor;
    uint8_t editor_count;
} _G;


void fps_camera_update(ct_world world,
                       ct_entity camera_ent,
                       float dt,
                       float dx,
                       float dy,
                       float updown,
                       float leftright,
                       float speed,
                       bool fly_mode) {

    CEL_UNUSED(dx);
    CEL_UNUSED(dy);

    float pos[3];
    float rot[4];
    float wm[16];

    auto transform = ct_transform_a0.get(world, camera_ent);

    ct_transform_a0.get_position(transform, pos);
    ct_transform_a0.get_rotation(transform, rot);
    ct_transform_a0.get_world_matrix(transform, wm);

    float x_dir[4];
    float z_dir[4];
    vec4_move(x_dir, &wm[0 * 4]);
    vec4_move(z_dir, &wm[2 * 4]);


    if (!fly_mode) {
        z_dir[1] = 0.0f;
    }

    // POS
    float x_dir_new[3];
    float z_dir_new[3];

    vec3_mul(x_dir_new, x_dir, dt * leftright * speed);
    vec3_mul(z_dir_new, z_dir, dt * updown * speed);

    vec3_add(pos, pos, x_dir_new);
    vec3_add(pos, pos, z_dir_new);

    // ROT
//    float rotation_around_world_up[4];
//    float rotation_around_camera_right[4];
//
//
//
//    local rotation_around_world_up = Quatf.from_axis_angle(Vec3f.unit_y(), -dx * dt * 100)
//    local rotation_around_camera_right = Quatf.from_axis_angle(x_dir, dy * dt * 100)
//    local rotation = rotation_around_world_up * rotation_around_camera_right
//
//    Transform.set_position(self.transform, pos)
//    Transform.set_rotation(self.transform, rot * rotation)
//    end

    ct_transform_a0.set_position(transform, pos);
}

void on_debugui() {
    char dock_id[128] = {};

    _G.active_editor = UINT8_MAX;

    for (uint8_t i = 0; i < _G.editor_count; ++i) {
        if (_G.is_level[i]) {
            snprintf(dock_id, CETECH_ARRAY_LEN(dock_id),
                     "Level %s###level_editor_%d", _G.path[i], i + 1);
        } else {
            snprintf(dock_id, CETECH_ARRAY_LEN(dock_id),
                     "Entity %s###level_editor_%d", _G.path[i], i + 1);
        }

        if (ct_debugui_a0.BeginDock(dock_id,
                                    &_G.visible[i],
                                    DebugUIWindowFlags_(
                                            DebugUIWindowFlags_NoScrollbar))) {

            if (ct_debugui_a0.IsMouseHoveringWindow()) {
                _G.active_editor = i;

                float proj[16], view[16];
                float size[2];
                ct_debugui_a0.GetWindowSize(size);

                ct_camera camera = ct_camera_a0.get(_G.world[i],
                                                    _G.camera_ent[i]);
                ct_camera_a0.get_project_view(camera, proj, view,
                                              static_cast<int>(size[0]),
                                              static_cast<int>(size[1]));

//                static float im[16] = {
//                        1.0f, 0.0f, 0.0f, 0.0f,
//                        0.0f, 1.0f, 0.0f, 0.0f,
//                        0.0f, 0.0f, 1.0f, 0.0f,
//                        0.0f, 0.0f, 0.0f, 1.0f,
//                };
//                auto origin = ImGui::GetItemRectMin();
//                ImGuizmo::SetRect(origin.x, origin.y, size[0], size[1]);
//                ImGuizmo::Manipulate(view, proj, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, im);

                if (ct_debugui_a0.IsMouseClicked(0, false)) {
                    ct_explorer_a0.set_level(_G.world[i], _G.entity[i],
                                                    _G.entity_name[i],
                                                    _G.root[i], _G.path[i],
                                                    false);
                }
            }

            auto th = ct_viewport_a0.get_local_resource(
                    _G.viewport[i], CT_ID64_0("bb_color"));

            float size[2];
            ct_debugui_a0.GetWindowSize(size);
            ct_viewport_a0.resize(_G.viewport[i], size[0], size[1]);
            ct_debugui_a0.Image2(th,
                                 size,
                                 (float[2]) {0.0f, 0.0f},
                                 (float[2]) {1.0f, 1.0f},
                                 (float[4]) {1.0f, 1.0f, 1.0f, 1.0f},
                                 (float[4]) {0.0f, 0.0f, 0.0, 0.0f});
        } else {

        }
        ct_debugui_a0.EndDock();

    }
}

void render() {
    for (uint8_t i = 0; i < _G.editor_count; ++i) {
        if (!_G.visible[i]) {
            continue;
        }

        ct_camera camera = ct_camera_a0.get(_G.world[i], _G.camera_ent[i]);
        ct_viewport_a0.render_world(_G.world[i], camera, _G.viewport[i]);
    }
}

uint32_t find_level(uint64_t name) {
    for (uint32_t i = 0; i < MAX_EDITOR; ++i) {
        if (_G.entity_name[i] != name) {
            continue;
        }

        return i;
    }

    return UINT32_MAX;
}

static void open(uint64_t name,
                 uint64_t root,
                 const char *path,
                 bool is_level) {
    uint32_t level_idx = find_level(name);

    int idx = _G.editor_count;
    ++_G.editor_count;

    _G.visible[idx] = true;
    _G.viewport[idx] = ct_viewport_a0.create(
            CT_ID64_0("default"), 0, 0);

    if (UINT32_MAX != level_idx) {
        _G.world[idx] = _G.world[level_idx];
    } else {
        _G.world[idx] = ct_world_a0.create();

        if (is_level) {
            _G.entity[idx] = ct_level_a0.load_level(_G.world[idx], name);
        } else {
            _G.entity[idx] = ct_entity_a0.spawn(_G.world[idx], name);
        }

        _G.is_first[idx] = true;
        _G.is_level[idx] = is_level;
    }
    _G.camera_ent[idx] = ct_entity_a0.spawn(_G.world[idx],
                                            CT_ID64_0("content/camera"));

    _G.path[idx] = strdup(path);
    _G.root[idx] = root;
    _G.entity_name[idx] = name;

    ct_explorer_a0.set_level(_G.world[idx], _G.entity[idx],
                                    _G.entity_name[idx], _G.root[idx],
                                    _G.path[idx], is_level);
}

void init() {
}

void shutdown() {
}

void update(float dt) {
    if (UINT8_MAX == _G.active_editor) {
        return;
    }

    float updown = 0.0f;
    float leftright = 0.0f;

    auto up_key = ct_keyboard_a0.button_index("w");
    auto down_key = ct_keyboard_a0.button_index("s");
    auto left_key = ct_keyboard_a0.button_index("a");
    auto right_key = ct_keyboard_a0.button_index("d");

    if (ct_keyboard_a0.button_state(0, up_key) > 0) {
        updown = 1.0f;
    }

    if (ct_keyboard_a0.button_state(0, down_key) > 0) {
        updown = -1.0f;
    }

    if (ct_keyboard_a0.button_state(0, right_key) > 0) {
        leftright = 1.0f;
    }

    if (ct_keyboard_a0.button_state(0, left_key) > 0) {
        leftright = -1.0f;
    }

    fps_camera_update(_G.world[_G.active_editor],
                      _G.camera_ent[_G.active_editor], dt,
                      0, 0,
                      updown, leftright, 10.0f, false);
}

static ct_level_view_a0 level_api = {
//            .register_module = playground::register_module,
//            .unregister_module = playground::unregister_module,
};

static void on_asset_double_click(uint64_t type,
                                  uint64_t name,
                                  uint64_t root,
                                  const char *path) {
    if (CT_ID64_0("level") == type) {
        open(name, root, path, true);
        return;
    }

    if (CT_ID64_0("entity") == type) {
        open(name, root, path, false);
        return;
    }
}

static void _init(ct_api_a0 *api) {
    _G = {
            .active_editor = UINT8_MAX
    };

    ct_playground_a0.register_module(
            CT_ID64_0("level_editor"),
            (ct_playground_module_fce) {
                    .on_init = init,
                    .on_shutdown = shutdown,
                    .on_render = render,
                    .on_update = update,
                    .on_ui = on_debugui,
            });

    ct_asset_browser_a0.register_on_asset_double_click(on_asset_double_click);

    api->register_api("ct_level_view_a0", &level_api);
}

static void _shutdown() {

    ct_playground_a0.unregister_module(
            CT_ID64_0("level_editor")
    );


    ct_asset_browser_a0.unregister_on_asset_double_click(on_asset_double_click);

    _G = {};
}

CETECH_MODULE_DEF(
        level_view,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_hash_a0);
            CETECH_GET_API(api, ct_renderer_a0);
            CETECH_GET_API(api, ct_debugui_a0);
            CETECH_GET_API(api, ct_app_a0);
            CETECH_GET_API(api, ct_world_a0);
            CETECH_GET_API(api, ct_level_a0);
            CETECH_GET_API(api, ct_entity_a0);
            CETECH_GET_API(api, ct_camera_a0);
            CETECH_GET_API(api, ct_transform_a0);
            CETECH_GET_API(api, ct_keyboard_a0);
            CETECH_GET_API(api, ct_viewport_a0);
            CETECH_GET_API(api, ct_asset_browser_a0);
            CETECH_GET_API(api, ct_explorer_a0);
            CETECH_GET_API(api, ct_playground_a0);
            CETECH_GET_API(api, ct_ydb_a0);
        },
        {
            CEL_UNUSED(reload);
            _init(api);
        },
        {
            CEL_UNUSED(reload);
            CEL_UNUSED(api);
            _shutdown();
        }
)