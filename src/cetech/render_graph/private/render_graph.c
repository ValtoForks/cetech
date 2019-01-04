//==============================================================================
// includes
//==============================================================================

#include <celib/allocator.h>
#include <celib/api_system.h>
#include <celib/memory.h>
#include <celib/module.h>
#include <celib/hashlib.h>
#include <celib/array.inl>
#include <celib/hash.inl>
#include <celib/ebus.h>

#include <cetech/renderer/renderer.h>
#include <cetech/renderer/gfx.h>
#include <cetech/debugdraw/debugdraw.h>
#include <cetech/ecs/ecs.h>
#include <cetech/kernel/kernel.h>
#include <cetech/transform/transform.h>
#include <celib/cdb.h>
#include "cetech/render_graph/render_graph.h"


#define _G render_graph_global

//==============================================================================
// GLobals
//==============================================================================

static struct _G {
    struct ce_alloc *alloc;
} _G;

#include "graph.h"
#include "module.h"
#include "builder.h"

static struct ct_rg_a0 render_graph_api = {
        .create_graph = create_render_graph,
        .destroy_graph = destroy_render_graph,
        .create_module = create_module,
        .destroy_module = destroy_module,
        .create_builder = create_render_builder,
        .destroy_builder = destroy_render_builder,
};

struct ct_rg_a0 *ct_rg_a0 = &render_graph_api;


static void _init(struct ce_api_a0 *api) {
    CE_UNUSED(api);
    _G = (struct _G) {
            .alloc = ce_memory_a0->system,
    };

    api->register_api("ct_rg_a0", &render_graph_api);

}

static void _shutdown() {
    _G = (struct _G) {};
}

CE_MODULE_DEF(
        render_graph,
        {
            CE_INIT_API(api, ce_id_a0);
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ct_renderer_a0);
            CE_INIT_API(api, ce_ebus_a0);
            CE_INIT_API(api, ct_dd_a0);
        },
        {
            CE_UNUSED(reload);
            _init(api);
        },
        {
            CE_UNUSED(reload);
            CE_UNUSED(api);

            _shutdown();
        }
)