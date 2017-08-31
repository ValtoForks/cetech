#ifndef CETECH_LEVEL_INSPECTOR_H
#define CETECH_LEVEL_INSPECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>


struct ct_level;
struct ct_world;

//==============================================================================
// Typedefs
//==============================================================================
typedef void (*ct_li_on_entity)(struct ct_world world, struct ct_level level, uint64_t name);

//==============================================================================
// Api
//==============================================================================

struct ct_level_inspector_a0 {
    void (*set_level)(struct ct_world world, struct ct_level level, uint64_t name, uint64_t root, const char* path);

    void (*register_on_entity_click)(ct_li_on_entity on_entity);
    void (*unregister_on_entity_click)(ct_li_on_entity on_entity);
};

#ifdef __cplusplus
}
#endif

#endif //CETECH_LEVEL_INSPECTOR_H