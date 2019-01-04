#include <stdbool.h>

#include <celib/array.inl>
#include <celib/hashlib.h>
#include <celib/memory.h>
#include <celib/api_system.h>
#include <celib/module.h>
#include <celib/cdb.h>

#include <cetech/debugui/debugui.h>
#include <cetech/editor/property.h>
#include <stdio.h>
#include <celib/fs.h>
#include <cetech/editor/resource_browser.h>
#include <cetech/debugui/icons_font_awesome.h>

#include "../dock.h"

#define _G explorer_globals
static struct _G {
    uint64_t docks_n;
    uint64_t *docks;

    uint64_t *contexts;

    struct ce_alloc *allocator;
} _G;

#define MAX_CONTEXT 8

#define _PROP_DOCK_ID\
    CE_ID64_0("dock_id", 0x4a6df3bdedc53da2ULL)


struct ct_dock_i0 *_find_dock_i(uint64_t type) {
    struct ce_api_entry it = ce_api_a0->first(DOCK_INTERFACE);

    while (it.api) {
        struct ct_dock_i0 *i = (it.api);

        if (i && i->cdb_type
            && (i->cdb_type() == type)) {
            return i;
        }

        it = ce_api_a0->next(it);
    }

    return NULL;
}

uint64_t create_dock(uint64_t type,
                     bool visible) {

    struct ct_dock_i0 *i = _find_dock_i(type);

    if (!i) {
        return 0;
    }

    uint64_t obj = ce_cdb_a0->create_object(ce_cdb_a0->db(), type);

    uint64_t flags = 0;

    if (i->dock_flags) {
        flags = i->dock_flags();
    }

    ce_cdb_obj_o *w = ce_cdb_a0->write_begin(obj);
    ce_cdb_a0->set_bool(w, PROP_DOCK_VISIBLE, visible);
    ce_cdb_a0->set_uint64(w, PROP_DOCK_flags, (uint64_t) flags);
    ce_cdb_a0->set_uint64(w, _PROP_DOCK_ID, _G.docks_n++);
    ce_cdb_a0->write_commit(w);

    ce_array_push(_G.docks, obj, _G.allocator);

    return obj;
}


void draw_all() {
    uint64_t n = ce_array_size(_G.docks);

    for (int i = 0; i < n; ++i) {
        uint64_t dock = _G.docks[i];
        uint64_t type = ce_cdb_a0->obj_type(dock);
        struct ct_dock_i0 *di = _find_dock_i(type);

        if (di) {
            const ce_cdb_obj_o *reader = ce_cdb_a0->read(dock);
            uint64_t id = ce_cdb_a0->read_uint64(reader, _PROP_DOCK_ID, 0);
            char title[128] = {};
            snprintf(title, CE_ARRAY_LEN(title), "%s##%s_dock%llu",
                     di->display_title(dock), di->name(dock), id);

            uint64_t flags = ce_cdb_a0->read_uint64(reader, PROP_DOCK_flags, 0);
            bool visible = ce_cdb_a0->read_bool(reader, PROP_DOCK_VISIBLE, false);

            if (ct_debugui_a0->BeginDock(title, &visible, flags)) {
                if (di->draw_menu) {
                    di->draw_menu(dock);
                }

                if (di->draw_ui) {
                    di->draw_ui(dock);
                }
            }
            ct_debugui_a0->EndDock();
        }
    }
}

static const char *display_name(struct ct_dock_i0 *i) {
    const char *name = NULL;

    if (i->display_title) {
        name = i->display_title(0);
    }

    if (!name) {
        name = "(no name)";
    }
    return name;
}

static void draw_menu() {
    const uint64_t n = ce_array_size(_G.docks);

    if (ct_debugui_a0->BeginMenu("Docks", true)) {

        if (ct_debugui_a0->BeginMenu(ICON_FA_PLUS" ""Add new dock", true)) {
            struct ce_api_entry it = ce_api_a0->first(DOCK_INTERFACE);

            while (it.api) {
                struct ct_dock_i0 *i = (it.api);

                const char *name = display_name(i);

                if (ct_debugui_a0->MenuItem(name, NULL, false, true)) {
                    create_dock(i->cdb_type(), true);
                }

                it = ce_api_a0->next(it);
            }

            ct_debugui_a0->EndMenu();
        }

        if (ct_debugui_a0->BeginMenu("Layout", true)) {
            if (ct_debugui_a0->MenuItem("Save", NULL, false, true)) {
                struct ce_vio *f = ce_fs_a0->open(RESOURCE_BROWSER_SOURCE,
                                                  "core/default.dock_layout",
                                                  FS_OPEN_WRITE);
                ct_debugui_a0->SaveDock(f);
                ce_fs_a0->close(f);
            }

            if (ct_debugui_a0->MenuItem("Load", NULL, false, true)) {
                ct_debugui_a0->LoadDock("core/default.dock_layout");
            }
            ct_debugui_a0->EndMenu();
        }

        ct_debugui_a0->Separator();

        for (int i = 0; i < n; ++i) {
            uint64_t dock = _G.docks[i];
            uint64_t type = ce_cdb_a0->obj_type(dock);
            struct ct_dock_i0 *di = _find_dock_i(type);

            const ce_cdb_obj_o *reader = ce_cdb_a0->read(dock);
            uint64_t contextid = ce_cdb_a0->read_uint64(reader, PROP_DOCK_CONTEXT, 0);

            const char *name = display_name(di);
            if (ct_debugui_a0->BeginMenu(name, true)) {
                if (ct_debugui_a0->BeginMenu("Context", true)) {
                    for (int j = 0; j < MAX_CONTEXT; ++j) {
                        char context_str[2] = {'0' + j};

                        bool selected = contextid == j;

                        if (ct_debugui_a0->MenuItem(context_str, NULL,
                                                    selected, true)) {
                            ce_cdb_obj_o *w = ce_cdb_a0->write_begin(dock);
                            ce_cdb_a0->set_uint64(w, PROP_DOCK_CONTEXT, j);
                            ce_cdb_a0->write_commit(w);
                        }
                    }

                    ct_debugui_a0->EndMenu();
                }
                ct_debugui_a0->EndMenu();
            }
        }

        ct_debugui_a0->EndMenu();
    }

    for (int i = 0; i < n; ++i) {
        uint64_t dock = _G.docks[i];
        uint64_t type = ce_cdb_a0->obj_type(dock);
        struct ct_dock_i0 *di = _find_dock_i(type);

        if (di->draw_main_menu) {
            di->draw_main_menu(dock);
        }

    }
}

static uint64_t create_context(const char *name) {
    const uint64_t context_id = ce_id_a0->id64(name);
    const uint64_t context_idx = ce_array_size(_G.contexts);
    ce_array_push(_G.contexts, context_id, _G.allocator);
    return context_idx;
}

struct ct_dock_a0 dock_a0 = {
        .draw_all = draw_all,
        .draw_menu = draw_menu,
        .create_dock = create_dock,
};

struct ct_dock_a0 *ct_dock_a0 = &dock_a0;

static void _init(struct ce_api_a0 *api) {
    _G = (struct _G) {
            .allocator = ce_memory_a0->system,
    };

    ce_api_a0->register_api("ct_dock_a0", ct_dock_a0);

    create_context("");
}

static void _shutdown() {
    _G = (struct _G) {};
}

CE_MODULE_DEF(
        dock,
        {
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ce_module_a0);
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