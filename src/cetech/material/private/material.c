//==============================================================================
// Include
//==============================================================================
#include <stdio.h>
#include <string.h>

#include "celib/allocator.h"

#include "celib/hashlib.h"
#include "celib/memory.h"
#include "celib/api_system.h"


#include "cetech/resource/resource.h"

#include <cetech/renderer/renderer.h>
#include <cetech/renderer/gfx.h>
#include <cetech/material/material.h>
#include <cetech/shader/shader.h>

#include <cetech/texture/texture.h>
#include <celib/module.h>

#include <celib/os.h>
#include <celib/macros.h>
#include <cetech/debugui/debugui.h>
#include <cetech/ecs/ecs.h>
#include <cetech/mesh/mesh_renderer.h>
#include <cetech/editor/resource_preview.h>
#include <cetech/editor/resource_ui.h>
#include <celib/log.h>
#include <cetech/resource/builddb.h>
#include <bgfx/defines.h>
#include <cetech/editor/property.h>
#include <stdlib.h>

#include "material.h"

int materialcompiler_init(struct ce_api_a0 *api);

//==============================================================================
// Defines
//==============================================================================

#define _G MaterialGlobals
#define LOG_WHERE "material"


//==============================================================================
// GLobals
//==============================================================================

static struct _G {
    struct ce_cdb_t db;
    struct ce_alloc *allocator;
} _G;


//==============================================================================
// Resource
//==============================================================================

static ct_render_uniform_type_t _type_to_bgfx[] = {
        [MAT_VAR_NONE] = CT_RENDER_UNIFORM_TYPE_COUNT,
        [MAT_VAR_INT] = CT_RENDER_UNIFORM_TYPE_INT1,
        [MAT_VAR_TEXTURE] = CT_RENDER_UNIFORM_TYPE_INT1,
        [MAT_VAR_TEXTURE_HANDLER] = CT_RENDER_UNIFORM_TYPE_INT1,
        [MAT_VAR_VEC4] = CT_RENDER_UNIFORM_TYPE_VEC4,
};

static uint64_t _str_to_type(const char *type) {
    if (!strcmp(type, "texture")) {
        return MAT_VAR_TEXTURE;
    } else if (!strcmp(type, "texture_handler")) {
        return MAT_VAR_TEXTURE_HANDLER;
    } else if ((!strcmp(type, "color")) || (!strcmp(type, "vec4"))) {
        return (!strcmp(type, "color")) ? MAT_VAR_COLOR4 : MAT_VAR_VEC4;
    }
    return MAT_VAR_NONE;
}

static void online(uint64_t name,
                   uint64_t obj) {
    CE_UNUSED(name);

    const ce_cdb_obj_o *reader = ce_cdb_a0->read(obj);

    uint64_t layers_obj = ce_cdb_a0->read_subobject(reader, MATERIAL_LAYERS, 0);
    const ce_cdb_obj_o *layers_reader = ce_cdb_a0->read(layers_obj);

    const uint64_t layers_n = ce_cdb_a0->prop_count(layers_obj);
    const uint64_t *layers_keys = ce_cdb_a0->prop_keys(layers_obj);

    for (int i = 0; i < layers_n; ++i) {
        uint64_t layer_obj = ce_cdb_a0->read_subobject(layers_reader,
                                                       layers_keys[i], 0);


        const ce_cdb_obj_o *layer_reader = ce_cdb_a0->read(layer_obj);

        uint64_t variables_obj = ce_cdb_a0->read_subobject(layer_reader,
                                                           MATERIAL_VARIABLES_PROP,
                                                           0);
        const uint64_t variables_n = ce_cdb_a0->prop_count(variables_obj);
        const uint64_t *variables_keys = ce_cdb_a0->prop_keys(variables_obj);

        const ce_cdb_obj_o *vars_reader = ce_cdb_a0->read(variables_obj);
        for (int k = 0; k < variables_n; ++k) {
            uint64_t var_obj = ce_cdb_a0->read_subobject(vars_reader,
                                                         variables_keys[k], 0);

            const ce_cdb_obj_o *var_reader = ce_cdb_a0->read(var_obj);

            const char *uniform_name = ce_cdb_a0->read_str(var_reader,
                                                           MATERIAL_VAR_NAME_PROP,
                                                           0);
            uint64_t type = _str_to_type(
                    ce_cdb_a0->read_str(var_reader, MATERIAL_VAR_TYPE_PROP,
                                        ""));

            if (!type) {
                continue;
            }

            const ct_render_uniform_handle_t handler = \
            ct_gfx_a0->create_uniform(uniform_name,
                                      _type_to_bgfx[type], 1);

            ce_cdb_obj_o *var_w = ce_cdb_a0->write_begin(var_obj);
            ce_cdb_a0->set_uint64(var_w, MATERIAL_VAR_HANDLER_PROP,
                                  handler.idx);
            ce_cdb_a0->write_commit(var_w);
        }
    }
}

static void offline(uint64_t name,
                    uint64_t obj) {
    CE_UNUSED(name, obj);
}

static uint64_t cdb_type() {
    return MATERIAL_TYPE;
}

static void ui_vec4(uint64_t var) {
    const ce_cdb_obj_o *reader = ce_cdb_a0->read(var);

    const char *str = ce_cdb_a0->read_str(reader, MATERIAL_VAR_NAME_PROP, "");
    ct_resource_ui_a0->ui_vec4(var,
                               (uint64_t[]) {MATERIAL_VAR_VALUE_PROP_X,
                                             MATERIAL_VAR_VALUE_PROP_Y,
                                             MATERIAL_VAR_VALUE_PROP_Z,
                                             MATERIAL_VAR_VALUE_PROP_W},
                               str, (struct ui_vec4_p0) {});
}

static void ui_color4(uint64_t var) {
    const ce_cdb_obj_o *reader = ce_cdb_a0->read(var);
    const char *str = ce_cdb_a0->read_str(reader, MATERIAL_VAR_NAME_PROP, "");
    ct_resource_ui_a0->ui_vec4(var,
                               (uint64_t[]) {MATERIAL_VAR_VALUE_PROP_X,
                                             MATERIAL_VAR_VALUE_PROP_Y,
                                             MATERIAL_VAR_VALUE_PROP_Z,
                                             MATERIAL_VAR_VALUE_PROP_W}, str,
                               (struct ui_vec4_p0) {.color=true});
}

static void ui_texture(uint64_t var) {
    const ce_cdb_obj_o *reader = ce_cdb_a0->read(var);

    const char *name = ce_cdb_a0->read_str(reader, MATERIAL_VAR_NAME_PROP,
                                           "");

    ct_resource_ui_a0->ui_resource(var, MATERIAL_VAR_VALUE_PROP, name,
                                   TEXTURE_TYPE, var);
}

static void draw_property(uint64_t material) {
    const ce_cdb_obj_o *reader = ce_cdb_a0->read(material);

    uint64_t layers_obj = ce_cdb_a0->read_ref(reader, MATERIAL_LAYERS, 0);

    if (!layers_obj) {
        return;
    }


    const ce_cdb_obj_o *layers_reader = ce_cdb_a0->read(layers_obj);

    uint64_t layer_count = ce_cdb_a0->prop_count(layers_obj);
    const uint64_t *layer_keys = ce_cdb_a0->prop_keys(layers_obj);
    for (int i = 0; i < layer_count; ++i) {
        uint64_t layer;
        layer = ce_cdb_a0->read_subobject(layers_reader, layer_keys[i], 0);


        const ce_cdb_obj_o *layer_reader = ce_cdb_a0->read(layer);

        const char *layer_name = ce_cdb_a0->read_str(layer_reader,
                                                     MATERIAL_LAYER_NAME, NULL);

        if (ct_debugui_a0->TreeNodeEx("Layer",
                                      DebugUITreeNodeFlags_DefaultOpen)) {
            ct_debugui_a0->NextColumn();
            ct_debugui_a0->Text("%s", layer_name);
            ct_debugui_a0->NextColumn();

            ct_resource_ui_a0->ui_str(layer, MATERIAL_LAYER_NAME,
                                      "Layer name", i);

            ct_resource_ui_a0->ui_resource(layer, MATERIAL_SHADER_PROP,
                                           "Shader",
                                           SHADER_TYPE, i);

            uint64_t variables;
            variables = ce_cdb_a0->read_ref(layer_reader,
                                            MATERIAL_VARIABLES_PROP, 0);

            const ce_cdb_obj_o *vars_reader = ce_cdb_a0->read(variables);

            uint64_t count = ce_cdb_a0->prop_count(variables);
            const uint64_t *keys = ce_cdb_a0->prop_keys(variables);

            for (int j = 0; j < count; ++j) {
                uint64_t var;
                var = ce_cdb_a0->read_subobject(vars_reader, keys[j], 0);

                if (!var) {
                    continue;
                }

                const ce_cdb_obj_o *var_reader = ce_cdb_a0->read(var);
                const char *type = ce_cdb_a0->read_str(var_reader,
                                                       MATERIAL_VAR_TYPE_PROP,
                                                       0);
                if (!type) continue;

                if (!strcmp(type, "texture")) {
                    ui_texture(var);
                } else if (!strcmp(type, "vec4")) {
                    ui_vec4(var);
                } else if (!strcmp(type, "color")) {
                    ui_color4(var);
                } else if (!strcmp(type, "mat4")) {
                }
            }

            ct_debugui_a0->TreePop();

        } else {
            ct_debugui_a0->NextColumn();
            ct_debugui_a0->Text("%s", layer_name);
            ct_debugui_a0->NextColumn();
        }
    }
}

static struct ct_entity load(uint64_t resource,
                             struct ct_world world) {

    struct ct_entity ent = ct_ecs_a0->spawn(world, 0x68373d25a0b84f58);

//    struct ct_mesh *mesh;
//    mesh = ct_ecs_a0->get_one(world, MESH_RENDERER_COMPONENT, ent);
//    mesh->material = resource;

    return ent;
}

static void unload(uint64_t resource,
                   struct ct_world world,
                   struct ct_entity entity) {
    ct_ecs_a0->destroy(world, &entity, 1);
}

static struct ct_resource_preview_i0 ct_resource_preview_i0 = {
        .load = load,
        .unload = unload,
};

static void *get_interface(uint64_t name_hash) {
    if (name_hash == RESOURCE_PREVIEW_I) {
        return &ct_resource_preview_i0;
    }
    return NULL;
}

uint64_t material_compiler(uint64_t k,
                           struct ct_resource_id rid,
                           const char *fullname);

static struct ct_resource_i0 ct_resource_i0 = {
        .cdb_type = cdb_type,
        .online = online,
        .offline = offline,
        .compilator = material_compiler,
        .get_interface = get_interface
};


//==============================================================================
// Interface
//==============================================================================

static uint64_t create(uint64_t name) {
    struct ct_resource_id rid = (struct ct_resource_id) {
            .uid = name,
    };

    uint64_t object = rid.uid;
    return object;
}

static uint64_t _find_slot_by_name(uint64_t variables,
                                   const char *name) {
    uint64_t k_n = ce_cdb_a0->prop_count(variables);
    const uint64_t *k = ce_cdb_a0->prop_keys(variables);
    for (int i = 0; i < k_n; ++i) {
        uint64_t key = k[i];

        const ce_cdb_obj_o *vars_reader = ce_cdb_a0->read(variables);
        uint64_t var = ce_cdb_a0->read_subobject(vars_reader, key, 0);

        const ce_cdb_obj_o *var_reader = ce_cdb_a0->read(var);
        const char *var_name = ce_cdb_a0->read_str(var_reader,
                                                   MATERIAL_VAR_NAME_PROP,
                                                   "");

        if (!strcmp(name, var_name)) {
            return var;
        }

    }

    return 0;
}

static void set_texture_handler(uint64_t material,
                                uint64_t layer,
                                const char *slot,
                                struct ct_render_texture_handle texture) {
    const ce_cdb_obj_o *mat_reader = ce_cdb_a0->read(material);

    uint64_t layers_obj = ce_cdb_a0->read_ref(mat_reader, MATERIAL_LAYERS, 0);
    const ce_cdb_obj_o *layers_reader = ce_cdb_a0->read(layers_obj);

    uint64_t layer_obj = ce_cdb_a0->read_ref(layers_reader, layer, 0);
    const ce_cdb_obj_o *layer_reader = ce_cdb_a0->read(layer_obj);

    uint64_t variables = ce_cdb_a0->read_ref(layer_reader,
                                             MATERIAL_VARIABLES_PROP,
                                             0);

    uint64_t var = _find_slot_by_name(variables, slot);
    if (!var) {
//        uint64_t name = ce_cdb_a0->read_uint64(mat_reader, RESOURCE_NAME_PROP, 0);

        ce_log_a0->warning(LOG_WHERE, "invalid slot: %s", slot);
        return;
    }

    ce_cdb_obj_o *writer = ce_cdb_a0->write_begin(var);
    ce_cdb_a0->set_uint64(writer, MATERIAL_VAR_VALUE_PROP, texture.idx);
    ce_cdb_a0->set_str(writer, MATERIAL_VAR_TYPE_PROP, "texture_handler");
    ce_cdb_a0->write_commit(writer);
}

static struct {
    uint64_t name;
    uint64_t e;
} _tbl[] = {
        {.name = 0, .e = 0},
        {.name = RENDER_STATE_RGB_WRITE, .e = BGFX_STATE_WRITE_RGB},
        {.name = RENDER_STATE_ALPHA_WRITE, .e = BGFX_STATE_WRITE_A},
        {.name = RENDER_STATE_DEPTH_WRITE, .e = BGFX_STATE_WRITE_Z},
        {.name = RENDER_STATE_DEPTH_TEST_LESS, .e = BGFX_STATE_DEPTH_TEST_LESS},
        {.name = RENDER_STATE_CULL_CCW, .e = BGFX_STATE_CULL_CCW},
        {.name = RENDER_STATE_CULL_CW, .e = BGFX_STATE_CULL_CW},
        {.name = RENDER_STATE_MSAA, .e = BGFX_STATE_MSAA},
};

uint64_t render_state_to_enum(uint64_t name) {

    for (uint32_t i = 1; i < CE_ARRAY_LEN(_tbl); ++i) {
        if (_tbl[i].name != name) {
            continue;
        }

        return _tbl[i].e;
    }

    return 0;
}


uint64_t _get_render_state(uint64_t layer) {
    const ce_cdb_obj_o *reader = ce_cdb_a0->read(layer);
    uint64_t render_state = ce_cdb_a0->read_subobject(reader,
                                                      ce_id_a0->id64(
                                                              "render_state"),
                                                      0);

    if (render_state) {
        uint64_t curent_render_state = 0;

        const uint64_t render_state_count = ce_cdb_a0->prop_count(render_state);
        const uint64_t *render_state_keys = ce_cdb_a0->prop_keys(render_state);

        for (uint32_t i = 0; i < render_state_count; ++i) {
            curent_render_state |= render_state_to_enum(render_state_keys[i]);
        }

        return curent_render_state;
    }

    return 0;
}


static void submit(uint64_t material,
                   uint64_t _layer,
                   uint8_t viewid) {
    const ce_cdb_obj_o *mat_reader = ce_cdb_a0->read(material);

    uint64_t layers_obj = ce_cdb_a0->read_ref(mat_reader, MATERIAL_LAYERS, 0);
    if (!layers_obj) {
        return;
    }

    const ce_cdb_obj_o *layers_reader = ce_cdb_a0->read(layers_obj);

    uint64_t layer = ce_cdb_a0->read_ref(layers_reader, _layer, 0);

    if (!layer) {
        return;
    }

    const ce_cdb_obj_o *layer_reader = ce_cdb_a0->read(layer);

    uint64_t variables = ce_cdb_a0->read_ref(layer_reader,
                                             MATERIAL_VARIABLES_PROP,
                                             0);

    uint64_t key_count = ce_cdb_a0->prop_count(variables);
    const uint64_t *keys = ce_cdb_a0->prop_keys(variables);

    uint8_t texture_stage = 0;

    const ce_cdb_obj_o *vars_reader = ce_cdb_a0->read(variables);

    for (int j = 0; j < key_count; ++j) {
        uint64_t var = ce_cdb_a0->read_ref(vars_reader, keys[j], 0);
        const ce_cdb_obj_o *var_reader = ce_cdb_a0->read(var);

        uint64_t type = _str_to_type(
                ce_cdb_a0->read_str(var_reader, MATERIAL_VAR_TYPE_PROP, ""));

        ct_render_uniform_handle_t handle = {
                .idx = (uint16_t) ce_cdb_a0->read_uint64(var_reader,
                                                         MATERIAL_VAR_HANDLER_PROP,
                                                         0)
        };

        switch (type) {
            case MAT_VAR_NONE:
                break;

            case MAT_VAR_INT: {
                uint64_t v = ce_cdb_a0->read_uint64(var_reader,
                                                    MATERIAL_VAR_VALUE_PROP, 0);
                ct_gfx_a0->set_uniform(handle, &v, 1);
            }
                break;

            case MAT_VAR_TEXTURE: {
                uint64_t tn = ce_cdb_a0->read_uint64(var_reader,
                                                     MATERIAL_VAR_VALUE_PROP,
                                                     0);

                ct_gfx_a0->set_texture(texture_stage++, handle,
                                       ct_texture_a0->get(tn), 0);
            }
                break;

            case MAT_VAR_TEXTURE_HANDLER: {
                uint64_t t = ce_cdb_a0->read_uint64(var_reader,
                                                    MATERIAL_VAR_VALUE_PROP, 0);
                ct_gfx_a0->set_texture(texture_stage++, handle,
                                       (ct_render_texture_handle_t) {.idx=(uint16_t) t},
                                       0);
            }
                break;

            case MAT_VAR_COLOR4:
            case MAT_VAR_VEC4: {
                float v[4] = {
                        ce_cdb_a0->read_float(var_reader,
                                              MATERIAL_VAR_VALUE_PROP_X, 1.0f),
                        ce_cdb_a0->read_float(var_reader,
                                              MATERIAL_VAR_VALUE_PROP_Y, 1.0f),
                        ce_cdb_a0->read_float(var_reader,
                                              MATERIAL_VAR_VALUE_PROP_Z, 1.0f),
                        ce_cdb_a0->read_float(var_reader,
                                              MATERIAL_VAR_VALUE_PROP_W, 1.0f)
                };

                ct_gfx_a0->set_uniform(handle, &v, 1);
            }
                break;

        }
    }

    uint64_t shader = ce_cdb_a0->read_ref(layer_reader,
                                          MATERIAL_SHADER_PROP,
                                          0);

    uint64_t shader_obj = shader;

    if (!shader_obj) {
        return;
    }

    ct_render_program_handle_t shaderp = ct_shader_a0->get(shader_obj);

    uint64_t state = _get_render_state(layer);

    ct_gfx_a0->set_state(state, 0);
    ct_gfx_a0->submit(viewid, shaderp, 0, false);
}

static struct ct_material_a0 material_api = {
        .create = create,
        .set_texture_handler = set_texture_handler,
        .submit = submit
};

struct ct_material_a0 *ct_material_a0 = &material_api;

static struct ct_property_editor_i0 ct_property_editor_i0 = {
        .cdb_type = cdb_type,
        .draw_ui = draw_property,
};

static int init(struct ce_api_a0 *api) {
    _G = (struct _G) {
            .allocator = ce_memory_a0->system,
            .db = ce_cdb_a0->db()
    };
    api->register_api("ct_material_a0", &material_api);

    ce_api_a0->register_api(RESOURCE_I_NAME, &ct_resource_i0);
    api->register_api(PROPERTY_EDITOR_INTERFACE_NAME, &ct_property_editor_i0);

    materialcompiler_init(api);

    return 1;
}

static void shutdown() {
    ce_cdb_a0->destroy_db(_G.db);
}

CE_MODULE_DEF(
        material,
        {
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ct_resource_a0);
            CE_INIT_API(api, ce_os_a0);
            CE_INIT_API(api, ce_id_a0);
            CE_INIT_API(api, ct_texture_a0);
            CE_INIT_API(api, ct_shader_a0);
            CE_INIT_API(api, ce_cdb_a0);
            CE_INIT_API(api, ct_renderer_a0);
        },
        {
            CE_UNUSED(reload);
            init(api);
        },
        {
            CE_UNUSED(reload);
            CE_UNUSED(api);

            shutdown();
        }
)