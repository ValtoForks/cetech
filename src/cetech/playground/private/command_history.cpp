#include <stdio.h>
#include "celib/map.inl"
#include "celib/fpumath.h"

#include <cetech/debugui/debugui.h>
#include <cetech/playground/playground.h>
#include <cetech/log/log.h>
#include <cetech/playground/command_history.h>
#include <cetech/playground/command_system.h>
#include <cetech/debugui/private/ocornut-imgui/imgui.h>

#include "cetech/hashlib/hashlib.h"
#include "cetech/os/memory.h"
#include "cetech/api/api_system.h"
#include "cetech/module/module.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_hash_a0);
CETECH_DECL_API(ct_debugui_a0);
CETECH_DECL_API(ct_playground_a0);
CETECH_DECL_API(ct_log_a0);
CETECH_DECL_API(ct_cmd_system_a0);

using namespace celib;

#define WINDOW_NAME "Command history"
#define PLAYGROUND_MODULE_NAME CT_ID64_0("command_history")

#define _G command_history_global
static struct _G {
    bool visible;
} _G;


static void ui_command_list() {
    const uint32_t command_count = ct_cmd_system_a0.command_count();
    const uint32_t current_idx = ct_cmd_system_a0.curent_idx();

    char buffer[128];
    char buffer2[128];

    for (uint32_t i = command_count; i > 0; --i) {
        ct_cmd_system_a0.command_text(buffer2, CETECH_ARRAY_LEN(buffer2), i);
        const bool is_selected = current_idx == i;

        snprintf(buffer, CETECH_ARRAY_LEN(buffer), "%s##cmd_%d", buffer2, i);

        if (ImGui::Selectable(buffer, is_selected)) {
            ct_cmd_system_a0.goto_idx(i);
            break;
        }
    }
}

static void on_debugui() {
    if (ct_debugui_a0.BeginDock(WINDOW_NAME,
                                &_G.visible,
                                DebugUIWindowFlags_(0))) {
        ui_command_list();
    }

    ct_debugui_a0.EndDock();

}

static void on_menu_window() {
    ct_debugui_a0.MenuItem2(WINDOW_NAME, NULL, &_G.visible, true);
}


static struct ct_command_history_a0 command_history_api = {
};

static void _init(ct_api_a0 *api) {
    _G = {
            .visible = true,
    };

    api->register_api("ct_command_history_a0", &command_history_api);

    ct_playground_a0.register_module(
            PLAYGROUND_MODULE_NAME,
            (ct_playground_module_fce) {
                    .on_ui = on_debugui,
                    .on_menu_window = on_menu_window,
            });
}

static void _shutdown() {
    ct_playground_a0.unregister_module(PLAYGROUND_MODULE_NAME);

    _G = {};
}

CETECH_MODULE_DEF(
        command_history,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_hash_a0);
            CETECH_GET_API(api, ct_debugui_a0);
            CETECH_GET_API(api, ct_playground_a0);
            CETECH_GET_API(api, ct_cmd_system_a0);
            CETECH_GET_API(api, ct_log_a0);
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