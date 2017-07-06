//==============================================================================
// Includes
//==============================================================================

#include <cetech/celib/map.inl>

#include <cetech/kernel/memory.h>
#include <cetech/kernel/config.h>
#include <cetech/modules/resource.h>

#include <cetech/kernel/module.h>


#include <cetech/modules/entity.h>

#include <cetech/kernel/yaml.h>
#include <cetech/kernel/api_system.h>

CETECH_DECL_API(ct_memory_api_v0);
CETECH_DECL_API(ct_world_api_v0);

using namespace cetech;

//==============================================================================
// Globals
//==============================================================================

namespace {
#define _G ComponentMaagerGlobals
    static struct ComponentMaagerGlobals {
        Map<ct_component_compiler_t> compiler_map;
        Map<uint32_t> spawn_order_map;
        Map<ct_component_clb> component_clb;
    } ComponentMaagerGlobals;
}


//==============================================================================
// Public interface
//==============================================================================

namespace component {

    void register_compiler(uint64_t type,
                           ct_component_compiler_t compiler,
                           uint32_t spawn_order) {
        map::set(_G.compiler_map, type, compiler);
        map::set(_G.spawn_order_map, type, spawn_order);
    }

    int compile(uint64_t type,
                yaml_node_t body,
                ct_blob_v0 *data) {

        ct_component_compiler_t compiler = map::get<ct_component_compiler_t>(
                _G.compiler_map, type, nullptr);

        if (!compiler) {
            return 0;
        }

        return compiler(body, data);
    }

    uint32_t get_spawn_order(uint64_t type) {
        return map::get(_G.spawn_order_map, type, (uint32_t) 0);
    }

    void register_type(uint64_t type,
                       struct ct_component_clb clb) {
        map::set(_G.component_clb, type, clb);

        ct_world_callbacks_t wclb = {
                .on_created = clb.on_world_create,
                .on_destroy = clb.on_world_destroy,
                .on_update = clb.on_world_update,
        };

        ct_world_api_v0.register_callback(wclb);
    }

    void spawn(world_t world,
               uint64_t type,
               entity_t *ent_ids,
               uint32_t *cent,
               uint32_t *ents_parent,
               uint32_t ent_count,
               void *data) {

        struct ct_component_clb clb = map::get(_G.component_clb, type,
                                            ct_component_clb_null);

        if (!clb.spawner) {
            return;
        }

        clb.spawner(world, ent_ids, cent, ents_parent, ent_count, data);
    }

    void destroy(world_t world,
                 entity_t *ent,
                 uint32_t count) {

        auto ct_it = map::begin(_G.component_clb);
        auto ct_end = map::end(_G.component_clb);

        while (ct_it != ct_end) {
            ct_it->value.destroyer(world, ent, count);
            ++ct_it;
        }
    }

    void set_property(uint64_t type,
                      world_t world,
                      entity_t entity,
                      uint64_t key,
                      struct ct_property_value value) {

        struct ct_component_clb clb = map::get(_G.component_clb,
                                            type, ct_component_clb_null);

        if (!clb.set_property) {
            return;
        }

        clb.set_property(world, entity, key, value);
    }

    ct_property_value get_property(uint64_t type,
                                world_t world,
                                entity_t entity,
                                uint64_t key) {

        struct ct_property_value value = {PROPERTY_INVALID};

        struct ct_component_clb clb = map::get(_G.component_clb,
                                            type, ct_component_clb_null);

        if (!clb.get_property) {
            return (struct ct_property_value) {PROPERTY_INVALID};
        }

        return clb.get_property(world, entity, key);
    }
}

namespace component_module {
    static struct ct_component_api_v0 api = {
            .register_compiler = component::register_compiler,
            .compile = component::compile,
            .spawn_order = component::get_spawn_order,
            .register_type = component::register_type,
            .spawn = component::spawn,
            .destroy = component::destroy,
            .set_property = component::set_property,
            .get_property = component::get_property
    };

    void _init_api(struct ct_api_v0 *api_v0) {
        api_v0->register_api("ct_component_api_v0", &api);
    }

    void _init(struct ct_api_v0 *api_v0) {
        _init_api(api_v0);

        CETECH_GET_API(api_v0, ct_memory_api_v0);
        CETECH_GET_API(api_v0, ct_world_api_v0);

        _G = {0};

        _G.compiler_map.init(ct_memory_api_v0.main_allocator());
        _G.spawn_order_map.init(ct_memory_api_v0.main_allocator());
        _G.component_clb.init(ct_memory_api_v0.main_allocator());
    }

    void _shutdown() {
        _G.compiler_map.destroy();
        _G.spawn_order_map.destroy();
        _G.component_clb.destroy();
    }

    extern "C" void component_load_module(struct ct_api_v0 *api) {
        _init(api);
    }

    extern "C" void component_unload_module(struct ct_api_v0 *api) {
        _shutdown();
    }

}