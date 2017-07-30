//==============================================================================
// Include
//==============================================================================

#include <cstdio>

#include "celib/allocator.h"
#include "celib/map.inl"
#include "celib/buffer.inl"

#include <cetech/engine/application/application.h>
#include "cetech/core/memory/memory.h"
#include "cetech/core/api/api_system.h"
#include "cetech/core/log/log.h"

#include "cetech/core/hashlib/hashlib.h"
#include "cetech/engine/machine/machine.h"

#include "cetech/engine/resource/resource.h"

#include <bgfx/bgfx.h>

#include "shader_blob.h"
#include "cetech/core/os/path.h"
#include "cetech/core/os/process.h"
#include "cetech/core/os/vio.h"

using namespace celib;
using namespace buffer;

namespace shader_compiler {
    int init(ct_api_a0 *api);
}

//==============================================================================
// GLobals
//==============================================================================

#define _G ShaderResourceGlobals
struct shader_blobResourceGlobals {
    Map<bgfx::ProgramHandle> handler_map;
    uint64_t type;
} ShaderResourceGlobals;


CETECH_DECL_API(ct_memory_a0)
CETECH_DECL_API(ct_resource_a0)
CETECH_DECL_API(ct_path_a0)
CETECH_DECL_API(ct_vio_a0)
CETECH_DECL_API(ct_process_a0)
CETECH_DECL_API(ct_log_a0)
CETECH_DECL_API(ct_hash_a0)


//==============================================================================
// Resource
//==============================================================================

namespace shader_resource {

    static const bgfx::ProgramHandle null_program = {};


    void *loader(ct_vio *input,
                 cel_alloc *allocator) {

        const int64_t size = input->size(input->inst);
        char *data = CEL_ALLOCATE(allocator, char, size);
        input->read(input->inst, data, 1, size);
        return data;
    }

    void unloader(void *new_data,
                  cel_alloc *allocator) {
        CEL_FREE(allocator, new_data);
    }

    void online(uint64_t name,
                void *data) {
        auto resource = shader_blob::get(data);

        auto vs_mem = bgfx::alloc(shader_blob::vs_size(resource));
        auto fs_mem = bgfx::alloc(shader_blob::fs_size(resource));

        memcpy(vs_mem->data, (resource + 1), resource->vs_size);
        memcpy(fs_mem->data, ((char *) (resource + 1)) + resource->vs_size,
               resource->fs_size);

        auto vs_shader = bgfx::createShader(vs_mem);
        auto fs_shader = bgfx::createShader(fs_mem);
        auto program = bgfx::createProgram(vs_shader, fs_shader, 1);

        map::set(_G.handler_map, name, program);
    }

    void offline(uint64_t name,
                 void *data) {
        CEL_UNUSED(data);

        auto program = map::get(_G.handler_map, name, null_program);

        if (program.idx == null_program.idx) {
            return;
        }

        bgfx::destroyProgram(program);

        map::remove(_G.handler_map, name);
    }

    void *reloader(uint64_t name,
                   void *old_data,
                   void *new_data,
                   cel_alloc *allocator) {
        offline(name, old_data);
        online(name, new_data);

        CEL_FREE(allocator, old_data);

        return new_data;
    }

    static const ct_resource_callbacks_t callback = {
            .loader = loader,
            .unloader =unloader,
            .online = online,
            .offline = offline,
            .reloader = reloader
    };

}

//==============================================================================
// Interface
//==============================================================================

namespace shader {
    int shader_init(ct_api_a0 *api) {
        CETECH_GET_API(api, ct_memory_a0);
        CETECH_GET_API(api, ct_resource_a0);
        CETECH_GET_API(api, ct_path_a0);
        CETECH_GET_API(api, ct_vio_a0);
        CETECH_GET_API(api, ct_process_a0);
        CETECH_GET_API(api, ct_log_a0);
        CETECH_GET_API(api, ct_hash_a0);

        _G = {};

        _G.type = ct_hash_a0.id64_from_str("shader");

        _G.handler_map.init(ct_memory_a0.main_allocator());

        ct_resource_a0.register_type(_G.type,
                                     shader_resource::callback);
#ifdef CETECH_CAN_COMPILE
        shader_compiler::init(api);
#endif

        return 1;
    }

    void shader_shutdown() {
        _G.handler_map.destroy();
    }

    bgfx::ProgramHandle shader_get(uint64_t name) {
        shader_blob::get(ct_resource_a0.get(_G.type, name));
        return map::get(_G.handler_map, name, shader_resource::null_program);
    }
}
    