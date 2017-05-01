#include <stddef.h>
#include <cetech/allocator.h>
#include <cetech/application.h>
#include <cetech/config.h>
#include <cetech/module.h>
#include "cetech/luasys.h"
#include "../luasys.h"

#define API_NAME "Application"


IMPORT_API(app_api_v0);

static int _application_quit(lua_State *l) {
    app_api_v0.quit();
    return 0;
}

static int _application_native_platform(lua_State *l) {
    const char *platform = app_api_v0.native_platform();

    luasys_push_string(l, platform);
    return 1;
}

static int _application_platform(lua_State *l) {
    const char *platform = app_api_v0.platform();

    luasys_push_string(l, platform);
    return 1;
}


void _register_lua_application_api(get_api_fce_t get_engine_api) {
    INIT_API(get_engine_api,app_api_v0, APPLICATION_API_ID);

    luasys_add_module_function(API_NAME, "quit", _application_quit);
    luasys_add_module_function(API_NAME, "get_native_platform",
                               _application_native_platform);
    luasys_add_module_function(API_NAME, "get_platform", _application_platform);
}