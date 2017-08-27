//==============================================================================
// Include
//==============================================================================

#include "celib/allocator.h"
#include "celib/map.inl"
#include "celib/buffer.inl"

#include "cetech/hashlib/hashlib.h"
#include "cetech/memory/memory.h"
#include "cetech/api/api_system.h"
#include "cetech/log/log.h"
#include "cetech/os/process.h"
#include "cetech/os/path.h"
#include "cetech/os/vio.h"
#include "cetech/resource/resource.h"
#include <cetech/module/module.h>
#include <cetech/playground/asset_property.h>
#include <cetech/debugui/debugui.h>
#include <cetech/renderer/texture.h>
#include <cetech/playground/entity_property.h>
#include <cetech/entity/entity.h>
#include <cetech/transform/transform.h>
#include <cfloat>

using namespace celib;
using namespace buffer;

//==============================================================================
// GLobals
//==============================================================================

#define _G TextureResourceGlobals
static struct _G {
} _G;

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_resource_a0);
CETECH_DECL_API(ct_path_a0);
CETECH_DECL_API(ct_vio_a0);
CETECH_DECL_API(ct_log_a0);
CETECH_DECL_API(ct_hash_a0);
CETECH_DECL_API(ct_asset_property_a0);
CETECH_DECL_API(ct_debugui_a0);
CETECH_DECL_API(ct_texture_a0);
CETECH_DECL_API(ct_entity_property_a0);
CETECH_DECL_API(ct_transform_a0);

static void on_component(struct ct_world world, struct ct_entity entity) {
    if(!ct_transform_a0.has(world, entity)) {
        return;
    }

    ct_transform t = ct_transform_a0.get(world, entity);

    float pos[3];
    ct_transform_a0.get_position(t, pos);

    ct_debugui_a0.DragFloat3("position", pos, 1.0f,FLT_MIN, FLT_MAX,"%.3f", 1.0f);

    ct_transform_a0.set_position(t, pos);
}

static int _init(ct_api_a0 *api) {
    _G = {};

#if CETECH_CAN_COMPILE
    ct_entity_property_a0.register_component(on_component);
#endif

    return 1;
}

static void _shutdown() {
#if CETECH_CAN_COMPILE
    ct_entity_property_a0.unregister_component(on_component);
#endif

    _G = {};
}


CETECH_MODULE_DEF(
        transform_property,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_resource_a0);
            CETECH_GET_API(api, ct_path_a0);
            CETECH_GET_API(api, ct_vio_a0);
            CETECH_GET_API(api, ct_log_a0);
            CETECH_GET_API(api, ct_hash_a0);
            CETECH_GET_API(api, ct_asset_property_a0);
            CETECH_GET_API(api, ct_debugui_a0);
            CETECH_GET_API(api, ct_texture_a0);
            CETECH_GET_API(api, ct_entity_property_a0);
            CETECH_GET_API(api, ct_transform_a0);
        },
        {
            _init(api);
        },
        {
            CEL_UNUSED(api);

            _shutdown();
        }
)