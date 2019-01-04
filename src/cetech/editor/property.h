#ifndef CETECH_PROPERTY_INSPECTOR_H
#define CETECH_PROPERTY_INSPECTOR_H

#include <stddef.h>
#include <stdint.h>

#define PROPERTY_EDITOR_INTERFACE_NAME \
    "ct_property_editor_i0"

#define PROPERTY_EDITOR_INTERFACE \
    CE_ID64_0("ct_property_editor_i0", 0x50566f03b5eacb95ULL)

struct ct_resource_id;

struct ct_property_editor_i0 {
    uint64_t (*cdb_type)();

    void (*draw_ui)(uint64_t obj);
    void (*draw_menu)(uint64_t obj);
};


struct ct_property_editor_a0 {
    void (*draw)(uint64_t obj);
};

CE_MODULE(ct_property_editor_a0);

#endif //CETECH_PROPERTY_INSPECTOR_H
