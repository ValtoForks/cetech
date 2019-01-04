#include <celib/hash.inl>
#include <celib/memory.h>
#include "celib/macros.h"
#include "celib/module.h"
#include "celib/api_system.h"

#include "cetech/editor/selcted_object.h"

#define _G editor_globals

static struct _G {
    struct ce_hash_t selected_object;
} _G;

static void set_selected_object(uint64_t context, uint64_t obj) {
    ce_hash_add(&_G.selected_object, context, obj, ce_memory_a0->system);
}

static uint64_t selected_object(uint64_t context) {
    return ce_hash_lookup(&_G.selected_object, context, 0);
}

static struct ct_selected_object_a0 ct_selected_object_api0 = {
        .selected_object = selected_object,
        .set_selected_object = set_selected_object,
};

struct ct_selected_object_a0 *ct_selected_object_a0 = &ct_selected_object_api0;


static void _init(struct ce_api_a0 *api) {
    _G = (struct _G) {
    };

    api->register_api("ct_selected_object_a0", &ct_selected_object_api0);
}

static void _shutdown() {
    _G = (struct _G) {};
}

CE_MODULE_DEF(
        selected_object,
        {
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
