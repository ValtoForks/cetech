#include <stddef.h>
#include "cetech/allocator.h"
#include <cetech/vio.h>

#include <cetech/config.h>
#include <cetech/module.h>
#include <cetech/world.h>
#include <cetech/camera.h>
#include <cetech/renderer.h>
#include "cetech/luasys.h"
#include "../luasys.h"

#define API_NAME "Renderer"

IMPORT_API(renderer_api_v0)

static int _renderer_render_world(lua_State *l) {
    world_t w = {.h = luasys_to_handler(l, 1)};
    camera_t c = {.idx = luasys_to_handler(l, 2)};

    renderer_api_v0.render_world(w, c, 0);

    return 0;
}

static int _renderer_set_debug(lua_State *l) {
    int debug = luasys_to_bool(l, 1);

    renderer_api_v0.set_debug(debug);
    return 0;
}

void _register_lua_renderer_api(get_api_fce_t get_engine_api) {
    renderer_api_v0 = *(struct renderer_api_v0 *) get_engine_api(RENDERER_API_ID);

    luasys_add_module_function(API_NAME, "render_world",
                               _renderer_render_world);
    luasys_add_module_function(API_NAME, "set_debug", _renderer_set_debug);
}