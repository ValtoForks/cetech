//==============================================================================
// Include
//==============================================================================

#include <celib/fpumath.h>
#include "celib/array.inl"

#include <cetech/yaml/ydb.h>
#include "cetech/macros.h"
#include "cetech/os/vio.h"
#include "cetech/os/path.h"
#include "cetech/os/memory.h"
#include "cetech/api/api_system.h"
#include "cetech/hashlib/hashlib.h"
#include "cetech/resource/resource.h"

#include "cetech/entity/entity.h"
#include "cetech/machine/machine.h"

#include <bgfx/defines.h>
#include <celib/array.h>

#include "material.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_resource_a0);
CETECH_DECL_API(ct_path_a0);
CETECH_DECL_API(ct_vio_a0);
CETECH_DECL_API(ct_hash_a0);
CETECH_DECL_API(ct_ydb_a0);
CETECH_DECL_API(ct_yng_a0);

using namespace celib;

#include "material_blob.h"

namespace material_compiler {
    namespace {
        struct material_compile_output {
            uint64_t* layer_names;
            uint64_t* shader_name;
            char* uniform_names;
            uint32_t* uniform_count;
            uint32_t* layer_offset;
            material_variable* var;
            uint64_t* render_state;
            uint64_t curent_render_state;
        };


        void _forach_variable_clb(const char *filename,
                                  uint64_t root_key,
                                  uint64_t key,
                                  material_compile_output &output) {
            cel_alloc* a = ct_memory_a0.main_allocator();

            uint64_t tmp_keys[] = {
                    root_key,
                    key,
                    ct_yng_a0.calc_key("name"),
            };

            const char *name = ct_ydb_a0.get_string(filename, tmp_keys,
                                                    CETECH_ARRAY_LEN(tmp_keys),
                                                    "");
            char uniform_name[32];
            strcpy(uniform_name, name);

            tmp_keys[2] = ct_yng_a0.calc_key("type");
            const char *type = ct_ydb_a0.get_string(filename, tmp_keys,
                                                    CETECH_ARRAY_LEN(tmp_keys),
                                                    "");

            material_variable mat_var = {};

            tmp_keys[2] = ct_yng_a0.calc_key("value");
            if (!strcmp(type, "texture")) {
                uint64_t texture_name = 0;

                //TODO : None type?
                if(ct_ydb_a0.has_key(filename, tmp_keys,
                                     CETECH_ARRAY_LEN(tmp_keys))) {
                        const char *v = ct_ydb_a0.get_string(
                                filename,
                                tmp_keys, CETECH_ARRAY_LEN(tmp_keys), "");
                    texture_name = CT_ID64_0(v);
                }

                mat_var.type = MAT_VAR_TEXTURE;
                mat_var.t = texture_name;

            } else if (!strcmp(type, "vec4")) {
                mat_var.type = MAT_VAR_VEC4;
                ct_ydb_a0.get_vec4(filename, tmp_keys,
                                   CETECH_ARRAY_LEN(tmp_keys), mat_var.v4,
                                   (float[4]) {0.0f});

            } else if (!strcmp(type, "mat4")) {
                mat_var.type = MAT_VAR_MAT44;
                ct_ydb_a0.get_mat4(filename, tmp_keys,
                                   CETECH_ARRAY_LEN(tmp_keys), mat_var.m44,
                                   (float[16]) {0.0f});
            }

            cel_array_push_n(output.var, &mat_var, 1, a);
            cel_array_push_n(output.uniform_names, uniform_name, CETECH_ARRAY_LEN(uniform_name), a);
        }
    }

    uint64_t render_state_to_enum(uint64_t name) {

        static struct {
            uint64_t name;
            uint64_t e;
        } _tbl[] = {
                {.name = CT_ID64_0(""), .e = 0},
                {.name = CT_ID64_0("rgb_write"), .e = BGFX_STATE_RGB_WRITE},
                {.name = CT_ID64_0("alpha_write"), .e = BGFX_STATE_ALPHA_WRITE},
                {.name = CT_ID64_0("depth_write"), .e = BGFX_STATE_DEPTH_WRITE},
                {.name = CT_ID64_0("depth_test_less"), .e = BGFX_STATE_DEPTH_TEST_LESS},
                {.name = CT_ID64_0("cull_ccw"), .e = BGFX_STATE_CULL_CCW},
                {.name = CT_ID64_0("msaa"), .e = BGFX_STATE_MSAA},
        };

        for (uint32_t i = 1; i < CETECH_ARRAY_LEN(_tbl); ++i) {
            if (_tbl[i].name != name) {
                continue;
            }

            return _tbl[i].e;
        }

        return _tbl[0].e;
    }


    void foreach_layer(const char *filename,
                       uint64_t root_key,
                       uint64_t key,
                       material_compile_output &output) {
        cel_alloc *a = ct_memory_a0.main_allocator();
        uint64_t tmp_keys[] = {
                root_key,
                key,
                ct_yng_a0.calc_key("shader"),
        };

        uint64_t tmp_key = ct_yng_a0.combine_key(tmp_keys,
                                                    CETECH_ARRAY_LEN(tmp_keys));

        const char *shader = ct_ydb_a0.get_string(filename, &tmp_key, 1, "");
        uint64_t shader_id = CT_ID64_0(shader);
        cel_array_push(output.shader_name, shader_id, a);

        auto layer_id = key;
        auto layer_offset = cel_array_size(output.var);

        tmp_keys[2] = ct_yng_a0.calc_key("render_state");
        tmp_key = ct_yng_a0.combine_key(tmp_keys,
                                           CETECH_ARRAY_LEN(tmp_keys));
        if (ct_ydb_a0.has_key(filename, &tmp_key, 1)) {
            output.curent_render_state = 0;

            uint64_t render_state_keys[32] = {};
            uint32_t render_state_count = 0;

            ct_ydb_a0.get_map_keys(filename,
                                   &tmp_key, 1,
                                   render_state_keys,
                                   CETECH_ARRAY_LEN(render_state_keys),
                                   &render_state_count);

            for (uint32_t i = 0; i < render_state_count; ++i) {
                output.curent_render_state |= render_state_to_enum(render_state_keys[i]);
            }
        }

        cel_array_push(output.layer_names, layer_id, a);
        cel_array_push(output.layer_offset, layer_offset, a);
        cel_array_push(output.render_state, output.curent_render_state, a);

        tmp_keys[2] = ct_yng_a0.calc_key("variables");
        tmp_key = ct_yng_a0.combine_key(tmp_keys,
                                           CETECH_ARRAY_LEN(tmp_keys));
        if (ct_ydb_a0.has_key(filename, &tmp_key, 1)) {
            uint64_t layers_keys[32] = {};
            uint32_t layers_keys_count = 0;

            ct_ydb_a0.get_map_keys(filename,
                                   &tmp_key, 1,
                                   layers_keys, CETECH_ARRAY_LEN(layers_keys),
                                   &layers_keys_count);

            for (uint32_t i = 0; i < layers_keys_count; ++i) {
                _forach_variable_clb(filename, tmp_key, layers_keys[i], output);
            }
        }

        cel_array_push(output.uniform_count,
                         cel_array_size(output.var) - layer_offset, a);

    };

    void compiler(const char *filename,
                 char**output_blob,
                 ct_compilator_api *compilator_api) {
        CEL_UNUSED(compilator_api);

        cel_alloc* a = ct_memory_a0.main_allocator();

        struct material_compile_output output = {};

        uint64_t key = ct_yng_a0.calc_key("layers");

        if (!ct_ydb_a0.has_key(filename, &key, 1)) {
            return;
        }

        uint64_t layers_keys[32] = {};
        uint32_t layers_keys_count = 0;

        ct_ydb_a0.get_map_keys(filename,
                               &key, 1,
                               layers_keys, CETECH_ARRAY_LEN(layers_keys),
                               &layers_keys_count);

        for (uint32_t i = 0; i < layers_keys_count; ++i) {
            foreach_layer(filename, key, layers_keys[i], output);
        }

        material_blob::blob_t resource = {
                .all_uniform_count = cel_array_size(output.var),
                .layer_count = cel_array_size(output.layer_names),
        };


        cel_array_push_n(*output_blob, &resource, sizeof(resource), a);

        cel_array_push_n(*output_blob, output.layer_names,
                         sizeof(uint64_t) * cel_array_size(output.layer_names), a);

        cel_array_push_n(*output_blob, output.shader_name,
                         sizeof(uint64_t)* cel_array_size(output.shader_name), a);

        cel_array_push_n(*output_blob, output.uniform_count,
                         sizeof(uint32_t)* cel_array_size(output.uniform_count), a);

        cel_array_push_n(*output_blob, output.render_state,
                         sizeof(uint64_t)* cel_array_size(output.render_state), a);

        cel_array_push_n(*output_blob, output.var,
                         sizeof(material_variable) * cel_array_size(output.var), a);

        cel_array_push_n(*output_blob, output.uniform_names,
                         sizeof(char)* cel_array_size(output.uniform_names), a);

        cel_array_push_n(*output_blob, output.layer_offset,
                         sizeof(uint32_t) * cel_array_size(output.layer_offset), a);

        cel_array_free(output.uniform_names, a);
        cel_array_free(output.layer_names, a);
        cel_array_free(output.uniform_count, a);
        cel_array_free(output.var, a);
        cel_array_free(output.layer_offset, a);
        cel_array_free(output.shader_name, a);
        cel_array_free(output.render_state, a);
    }

    int init(ct_api_a0 *api) {
        CETECH_GET_API(api, ct_memory_a0);
        CETECH_GET_API(api, ct_resource_a0);
        CETECH_GET_API(api, ct_path_a0);
        CETECH_GET_API(api, ct_vio_a0);
        CETECH_GET_API(api, ct_hash_a0);
        CETECH_GET_API(api, ct_yng_a0);
        CETECH_GET_API(api, ct_ydb_a0);

        ct_resource_a0.compiler_register(CT_ID64_0("material"),
                                         compiler, true);

        return 1;
    }
}