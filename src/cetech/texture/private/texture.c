//==============================================================================
// Include
//==============================================================================

#include "corelib/allocator.h"

#include "corelib/hashlib.h"
#include "corelib/memory.h"
#include "corelib/api_system.h"
#include "corelib/log.h"
#include "cetech/machine/machine.h"
#include <corelib/os.h>

#include "cetech/resource/resource.h"
#include "corelib/buffer.inl"

#include <corelib/module.h>
#include <cetech/texture/texture.h>
#include <cetech/renderer/renderer.h>
#include <corelib/config.h>
#include <corelib/ydb.h>
#include <cetech/asset_property/asset_property.h>
#include <cetech/debugui/debugui.h>
#include <cetech/ecs/entity_property.h>

//==============================================================================
// GLobals
//==============================================================================

#define _G TextureResourceGlobals
struct _G {
    uint32_t type;
    struct ct_alloc *allocator;
} _G;

//==============================================================================
// Compiler private
//==============================================================================


//==============================================================================
// Resource
//==============================================================================

#define TEXTURE_HANDLER_PROP CT_ID64_0("texture_handler")


static void _on_obj_change(uint64_t obj,
                           const uint64_t *prop,
                           uint32_t prop_count,
                           void *data);

void _texture_resource_online(uint64_t name,
                              struct ct_vio *input,
                              uint64_t obj) {

    const uint64_t size = input->size(input);
    char *data = CT_ALLOC(_G.allocator, char, size);
    input->read(input, data, 1, size);

    ct_cdb_a0->load(ct_cdb_a0->db(), data, obj, _G.allocator);

    ct_cdb_a0->register_notify(obj, _on_obj_change, NULL);

    uint64_t blob_size = 0;
    void *blob;
    blob = ct_cdb_a0->read_blob(obj, CT_ID64_0("texture_data"), &blob_size, 0);

    const ct_render_memory_t *mem = ct_renderer_a0->make_ref(blob, blob_size);

    ct_render_texture_handle_t texture;
    texture = ct_renderer_a0->create_texture(mem,
                                             CT_RENDER_TEXTURE_NONE,
                                             0, NULL);

    ct_cdb_obj_o *writer = ct_cdb_a0->write_begin(obj);
    ct_cdb_a0->set_uint64(writer, TEXTURE_HANDLER_PROP, texture.idx);
    ct_cdb_a0->write_commit(writer);
}


void _texture_resource_offline(uint64_t name,
                               uint64_t obj) {
    const uint64_t texture = ct_cdb_a0->read_uint64(obj, TEXTURE_HANDLER_PROP,
                                                    0);
    ct_renderer_a0->destroy_texture(
            (ct_render_texture_handle_t) {.idx=(uint16_t) texture});
}

static int _texturec(const char *input,
                     const char *output,
                     int gen_mipmaps,
                     int is_normalmap) {
    struct ct_alloc *alloc = ct_memory_a0->system;
    char *buffer = NULL;

    char *texturec = ct_resource_a0->compiler_external_join(alloc, "texturec");

    ct_buffer_printf(&buffer, alloc, "%s", texturec);
    ct_buffer_free(texturec, alloc);

    ct_buffer_printf(&buffer, alloc, " -f %s -o %s", input, output);

    if (gen_mipmaps) {
        ct_buffer_printf(&buffer, alloc, " %s", "--mips");
    }

    if (is_normalmap) {
        ct_buffer_printf(&buffer, alloc, " %s", "--normalmap");
    }

    ct_buffer_printf(&buffer, alloc, " %s", "2>&1");

    int status = ct_os_a0->process_a0->exec(buffer);

    ct_log_a0->info("application", "STATUS %d", status);

    return status;
}

static int _gen_tmp_name(char *tmp_filename,
                         const char *tmp_dir,
                         size_t max_len,
                         const char *filename) {

    struct ct_alloc *a = ct_memory_a0->system;

    char dir[1024] = {};
    ct_os_a0->path_a0->dir(dir, filename);

    char *tmp_dirname = NULL;
    ct_os_a0->path_a0->join(&tmp_dirname, a, 2, tmp_dir, dir);

    ct_os_a0->path_a0->make_path(tmp_dirname);

    int ret = snprintf(tmp_filename, max_len, "%s/%s.ktx", tmp_dirname,
                       ct_os_a0->path_a0->filename(filename));

    ct_buffer_free(tmp_dirname, a);

    return ret;
}

static void _compile(uint64_t obj) {
    const char *input = ct_cdb_a0->read_str(obj, CT_ID64_0("input"), "");
    bool gen_mipmaps = ct_cdb_a0->read_bool(obj, CT_ID64_0("gen_mipmaps"),
                                            false);
    bool is_normalmap = ct_cdb_a0->read_bool(obj, CT_ID64_0("is_normalmap"),
                                             false);

    struct ct_alloc *a = ct_memory_a0->system;

    const char *platform = ct_cdb_a0->read_str(ct_config_a0->config_object(),
                                               CT_ID64_0("kernel.platform"),
                                               "");

    char output_path[1024] = {};
    char tmp_filename[1024] = {};

    const char *source_dir = ct_resource_a0->compiler_get_source_dir();

    char *tmp_dir = ct_resource_a0->compiler_get_tmp_dir(a, platform);
    char *input_path = NULL;
    ct_os_a0->path_a0->join(&input_path, a, 2, source_dir, input);

    _gen_tmp_name(output_path, tmp_dir, CT_ARRAY_LEN(tmp_filename),
                  input);

    int result = _texturec(input_path, output_path, gen_mipmaps,
                           is_normalmap);

    if (result != 0) {
        return;
    }

    struct ct_vio *tmp_file = NULL;
    tmp_file = ct_os_a0->vio_a0->from_file(output_path, VIO_OPEN_READ);

    const uint64_t size = tmp_file->size(tmp_file);
    char *tmp_data = CT_ALLOC(ct_memory_a0->system, char, size + 1);
    tmp_file->read(tmp_file, tmp_data, sizeof(char), size);
    tmp_file->close(tmp_file);

    ct_cdb_obj_o *w = ct_cdb_a0->write_begin(obj);
    ct_cdb_a0->set_blob(w, CT_ID64_0("texture_data"), tmp_data, size);
    ct_cdb_a0->write_commit(w);
}


static void _on_obj_change(uint64_t obj,
                           const uint64_t *prop,
                           uint32_t prop_count,
                           void *data) {
    bool change = false;
    for (int i = 0; i < prop_count; ++i) {
        if (prop[i] == CT_ID64_0("gen_mipmaps")) {
            change = true;
            continue;
        }

        if (prop[i] == CT_ID64_0("is_normalmap")) {
            change = true;
            continue;
        }
    }

    if (change) {
        _compile(obj);

        uint64_t blob_size = 0;
        void *blob;
        blob = ct_cdb_a0->read_blob(obj, CT_ID64_0("texture_data"), &blob_size,
                                    0);
        const ct_render_memory_t *mem = ct_renderer_a0->make_ref(blob,
                                                                 blob_size);

        ct_render_texture_handle_t texture;
        texture = ct_renderer_a0->create_texture(mem,
                                                 CT_RENDER_TEXTURE_NONE,
                                                 0, NULL);

        ct_cdb_obj_o *writer = ct_cdb_a0->write_begin(obj);
        ct_cdb_a0->set_uint64(writer, TEXTURE_HANDLER_PROP, texture.idx);
        ct_cdb_a0->write_commit(writer);
    }
}

void texture_compiler(const char *filename,
                      char **output,
                      struct ct_compilator_api *compilator_api) {

    struct ct_alloc *a = ct_memory_a0->system;

    uint64_t key[] = {ct_yng_a0->key("input")};

    const char *input_str = ct_ydb_a0->get_str(filename, key, 1, "");

    key[0] = ct_yng_a0->key("gen_mipmaps");
    bool gen_mipmaps = ct_ydb_a0->get_bool(filename, key, 1, true);

    key[0] = ct_yng_a0->key("is_normalmap");
    bool is_normalmap = ct_ydb_a0->get_bool(filename, key, 1, false);

    uint64_t obj = ct_cdb_a0->create_object(ct_cdb_a0->db(),
                                            CT_ID64_0("texture"));

    ct_cdb_obj_o *w = ct_cdb_a0->write_begin(obj);
    ct_cdb_a0->set_str(w, CT_ID64_0("input"), input_str);
    ct_cdb_a0->set_bool(w, CT_ID64_0("gen_mipmaps"), gen_mipmaps);
    ct_cdb_a0->set_bool(w, CT_ID64_0("is_normalmap"), is_normalmap);
    ct_cdb_a0->write_commit(w);

    _compile(obj);

    ct_cdb_a0->dump(obj, output, a);
    ct_cdb_a0->destroy_object(obj);
    compilator_api->add_dependency(filename, input_str);
}

static uint64_t cdb_type() {
    return CT_ID32_0("texture");
}

static void draw_property(uint64_t obj) {

    ct_entity_property_a0->ui_str(obj, CT_ID64_0("input"),
                                  "Input", 0);

    ct_entity_property_a0->ui_bool(obj,
                                   CT_ID64_0("gen_mipmaps"),
                                   "Gen mipmaps");


    ct_entity_property_a0->ui_bool(obj,
                                   CT_ID64_0("is_normalmap"),
                                   "Is normalmap");

    ct_debugui_a0->LabelText("Texture preview", "");

    float size[2];
    ct_debugui_a0->GetContentRegionAvail(size);
    size[1] = size[0];

    ct_render_texture_handle_t texture;
    texture.idx = ct_cdb_a0->read_uint64(obj, TEXTURE_HANDLER_PROP, 0);
    ct_debugui_a0->Image(texture,
                         size,
                         (float[4]) {1.0f, 1.0f, 1.0f, 1.0f},
                         (float[4]) {0.0f, 0.0f, 0.0, 0.0f});
}

static const char *display_name() {
    return "Texture";
}

static struct ct_asset_property_i0 ct_asset_property_i0 = {
        .draw = draw_property,
        .display_name = display_name,
};

void *get_interface(uint64_t name_hash) {
    if (name_hash == ASSET_PROPERTY) {
        return &ct_asset_property_i0;
    }
    return NULL;
}

static struct ct_resource_i0 ct_resource_i0 = {
        .cdb_type = cdb_type,
        .get_interface = get_interface,
        .online =_texture_resource_online,
        .offline =_texture_resource_offline,
        .compilator = texture_compiler,
};


//==============================================================================
// Interface
//==============================================================================
int texture_init(struct ct_api_a0 *api) {
    _G = (struct _G) {
            .allocator = ct_memory_a0->system,
            .type = CT_ID32_0("texture")
    };

    ct_api_a0->register_api("ct_resource_i0", &ct_resource_i0);

    return 1;
}

void texture_shutdown() {
}

ct_render_texture_handle_t texture_get(uint32_t name) {
    struct ct_resource_id rid = {.type = _G.type, .name = name};
    uint64_t obj = ct_resource_a0->get(rid);

    if(!obj) {
        return (ct_render_texture_handle_t){.idx = UINT16_MAX};
    }
    struct ct_render_texture_handle texture = {
            .idx = (uint16_t) ct_cdb_a0->read_uint64(obj, TEXTURE_HANDLER_PROP,
                                                     0)
    };

    return texture;
}

static struct ct_texture_a0 texture_api = {
        .get = texture_get
};

struct ct_texture_a0 *ct_texture_a0 = &texture_api;

static void _init_api(struct ct_api_a0 *api) {

    api->register_api("ct_texture_a0", &texture_api);
}


CETECH_MODULE_DEF(
        texture,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_resource_a0);
            CETECH_GET_API(api, ct_os_a0);
            CETECH_GET_API(api, ct_log_a0);
            CETECH_GET_API(api, ct_hashlib_a0);
            CETECH_GET_API(api, ct_cdb_a0);
            CETECH_GET_API(api, ct_renderer_a0);
        },
        {
            CT_UNUSED(reload);
            _init_api(api);
            texture_init(api);
        },
        {
            CT_UNUSED(reload);
            CT_UNUSED(api);

            texture_shutdown();
        }
)