//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>

#include <cetech/celib/handler.inl>
#include <cetech/celib/map.inl>

#include <cetech/kernel/api_system.h>
#include <cetech/kernel/module.h>
#include <cetech/kernel/memory.h>
#include <cetech/kernel/yaml.h>
#include <cetech/kernel/hash.h>
#include <cetech/kernel/os.h>
#include <cetech/kernel/log.h>
#include <cetech/kernel/config.h>


#include <cetech/modules/resource.h>

#include <cetech/modules/entity.h>

#include <cetech/kernel/errors.h>

CETECH_DECL_API(memory_api_v0);
CETECH_DECL_API(component_api_v0);
CETECH_DECL_API(resource_api_v0);
CETECH_DECL_API(os_path_v0);
CETECH_DECL_API(log_api_v0);
CETECH_DECL_API(os_vio_api_v0);
CETECH_DECL_API(hash_api_v0);
CETECH_DECL_API(blob_api_v0);

using namespace cetech;

//==============================================================================
// Globals
//==============================================================================

#define _G EntityMaagerGlobals

static struct EntityMaagerGlobals {
    Handler<uint32_t> entity_handler;

//    Map<uint32_t> spawned_map;
//    Array<Array<entity_t>> spawned_array;

    uint64_t type;
} EntityMaagerGlobals;

struct entity_resource {
    uint32_t ent_count;
    uint32_t comp_type_count;
    //uint64_t parents [ent_count]
    //uint64_t comp_types [comp_type_count]
    //component_data cdata[comp_type_count]
};

struct component_data {
    uint32_t ent_count;
    uint32_t size;
    // uint32_t ent[ent_count];
    // char data[ent_count];
};

#define entity_resource_parents(r) ((uint32_t*)((r) + 1))
#define entity_resource_comp_types(r) ((uint64_t*)(entity_resource_parents(r) + ((r)->ent_count)))
#define entity_resource_comp_data(r) ((struct component_data*)(entity_resource_comp_types(r) + ((r)->comp_type_count)))

#define component_data_ent(cd) ((uint32_t*)((cd) + 1))
#define component_data_data(cd) ((char*)((component_data_ent(cd) + ((cd)->ent_count))))

//uint32_t _new_spawned_array() {
//    uint32_t idx = array::size(_G.spawned_array);
//
//
//    array::push_back(_G.spawned_array, {0});
//
//    Array<entity_t> *array = &_G.spawned_array[idx];
//    array->init(memory_api_v0.main_allocator());
//
//    return idx;
//}
//
//void _map_spawned_array(entity_t root,
//                        uint32_t idx) {
//
//    map::set(_G.spawned_map, root.h, idx);
//}
//
//Array<entity_t> &_get_spawned_array_by_idx(uint32_t idx) {
//    return _G.spawned_array[idx];
//}
//
//Array<entity_t> &_get_spawned_array(entity_t entity) {
//    uint32_t idx = map::get(_G.spawned_map, entity.h, UINT32_MAX);
//
//    return _G.spawned_array[idx];
//}
//
//
//void _destroy_spawned_array(entity_t entity) {
//    uint32_t idx = map::get(_G.spawned_map, entity.h, UINT32_MAX);
//    map::remove(_G.spawned_map, entity.h);
//
//    Array<entity_t> *array = &_G.spawned_array[idx];
//    array->destroy();
//}

//==============================================================================
// Compiler private
//==============================================================================
struct entity_compile_output {
    Map<uint32_t> component_ent;
    Array<Array<uint32_t>> component_ent_array;
    Map<uint32_t> entity_parent;
    Map<uint32_t> component_body;
    Array<Array<yaml_node_t>> component_body_array;
    Array<uint64_t> component_type;
    uint32_t ent_counter;
};

#ifdef CETECH_CAN_COMPILE

namespace entity_resource_compiler {
    static void preprocess(const char *filename,
                           yaml_node_t root,
                           struct compilator_api *capi) {
        auto a = memory_api_v0.main_allocator();
        const char *source_dir = resource_api_v0.compiler_get_source_dir();

        yaml_node_t prefab_node = yaml_get_node(root, "prefab");

        if (yaml_is_valid(prefab_node)) {
            char prefab_file[256] = {0};
            char prefab_str[256] = {0};
            yaml_as_string(prefab_node, prefab_str,
                           CETECH_ARRAY_LEN(prefab_str));

            snprintf(prefab_file, CETECH_ARRAY_LEN(prefab_file),
                     "%s.entity", prefab_str);

            capi->add_dependency(filename, prefab_file);


            char *full_path = os_path_v0.join(a, 2, source_dir, prefab_file);

            struct os_vio *prefab_vio = os_vio_api_v0.from_file(full_path,
                                                                VIO_OPEN_READ);

            char prefab_data[prefab_vio->size(prefab_vio->inst) + 1];
            memset(prefab_data, 0, prefab_vio->size(prefab_vio->inst) + 1);
            prefab_vio->read(prefab_vio->inst, prefab_data, sizeof(char),
                             prefab_vio->size(prefab_vio->inst));

            yaml_document_t h;
            yaml_node_t prefab_root = yaml_load_str(prefab_data, &h);

            preprocess(filename, prefab_root, capi);
            yaml_merge(root, prefab_root);

            prefab_vio->close(prefab_vio->inst);

            CETECH_FREE(a, full_path);
        }
    }

    struct foreach_children_data {
        int parent_ent;
        struct entity_compile_output *output;
    };


    static void compile_entitity(yaml_node_t rootNode,
                                 int parent,
                                 struct entity_compile_output *output);

    void forach_children_clb(yaml_node_t key,
                             yaml_node_t value,
                             void *_data) {
        struct foreach_children_data *data = (foreach_children_data *) _data;

        compile_entitity(value, data->parent_ent, data->output);
    }


    struct foreach_componets_data {
        struct entity_compile_output *output;
        uint32_t ent_id;
    };

    void foreach_components_clb(yaml_node_t key,
                                yaml_node_t value,
                                void *_data) {
        char uid_str[256] = {0};
        char component_type_str[256] = {0};
        uint64_t cid;
        int contain_cid = 0;

        struct foreach_componets_data *data = (foreach_componets_data *) _data;
        struct entity_compile_output *output = data->output;

        yaml_as_string(key, uid_str, CETECH_ARRAY_LEN(uid_str));

        yaml_node_t component_type_node = yaml_get_node(value,
                                                        "component_type");
        yaml_as_string(component_type_node, component_type_str,
                       CETECH_ARRAY_LEN(component_type_str));

        cid = hash_api_v0.id64_from_str(component_type_str);

        for (int i = 0; i < array::size(output->component_type); ++i) {
            if (output->component_type[i] != cid) {
                continue;
            }

            contain_cid = 1;
        }

        if (!contain_cid) {
            array::push_back(output->component_type, cid);

            uint32_t idx = array::size(output->component_ent_array);
            //Array<uint32_t> tmp_a(memory_api_v0.main_allocator());
            array::push_back(output->component_ent_array, {0});
            output->component_ent_array[idx].init(
                    memory_api_v0.main_allocator());

            map::set(output->component_ent, cid, idx);
        }

        if (!map::has(output->component_body, cid)) {
            uint32_t idx = array::size(output->component_body_array);
            //Array<yaml_node_t> tmp_a(memory_api_v0.main_allocator());
            array::push_back(output->component_body_array, {0});
            output->component_body_array[idx].init(
                    memory_api_v0.main_allocator());

            map::set(output->component_body, cid, idx);
        }

        uint32_t idx = map::get(output->component_ent, cid, UINT32_MAX);
        Array<uint32_t> &tmp_a = output->component_ent_array[idx];
        array::push_back(tmp_a, data->ent_id);

        idx = map::get(output->component_body, cid, UINT32_MAX);
        Array<yaml_node_t> &tmp_b = output->component_body_array[idx];
        array::push_back(tmp_b, value);
    }

    void compile_entitity(yaml_node_t rootNode,
                          int parent,
                          struct entity_compile_output *output) {

        uint32_t ent_id = output->ent_counter++;

        map::set(output->entity_parent, ent_id, (uint32_t) parent);

        yaml_node_t components_node = yaml_get_node(rootNode, "components");
        CETECH_ASSERT("entity_system", yaml_is_valid(components_node));

        struct foreach_componets_data data = {
                .ent_id = ent_id,
                .output = output
        };

        yaml_node_foreach_dict(components_node, foreach_components_clb, &data);

        yaml_node_t children_node = yaml_get_node(rootNode, "children");
        if (yaml_is_valid(children_node)) {
            int parent_ent = ent_id;
            //output->ent_counter += 1;

            struct foreach_children_data data = {
                    .parent_ent = parent_ent,
                    .output = output
            };

            yaml_node_foreach_dict(children_node, forach_children_clb, &data);
        }
    }


    struct entity_compile_output *create_output() {
        allocator *a = memory_api_v0.main_allocator();

        entity_compile_output *output = CETECH_ALLOCATE(a,
                                                        entity_compile_output,
                                                        sizeof(entity_compile_output));

        output->ent_counter = 0;

        output->component_type.init(a);
        output->component_ent.init(a);
        output->component_ent_array.init(a);
        output->entity_parent.init(a);
        output->component_body.init(a);
        output->component_body_array.init(a);

        return output;
    }

    void destroy_output(struct entity_compile_output *output) {
        output->component_type.destroy();
        output->entity_parent.destroy();

        // clean inner array
        auto ce_it = array::begin(output->component_ent_array);
        auto ce_end = array::end(output->component_ent_array);
        while (ce_it != ce_end) {
            ce_it->destroy();
            ++ce_it;
        }

        output->component_ent.destroy();
        output->component_ent_array.destroy();

        // clean inner array
        auto cb_it = array::begin(output->component_body_array);
        auto cb_end = array::end(output->component_body_array);

        while (cb_it != cb_end) {
            cb_it->destroy();
            ++cb_it;
        }

        output->component_body.destroy();
        output->component_body_array.destroy();

        allocator *a = memory_api_v0.main_allocator();
        CETECH_FREE(a, output);
    }

    void compile_entity(struct entity_compile_output *output,
                        yaml_node_t root,
                        const char *filename,
                        struct compilator_api *compilator_api) {

        preprocess(filename, root, compilator_api);
        compile_entitity(root, UINT32_MAX, output);
    }

    uint32_t ent_counter(struct entity_compile_output *output) {
        return output->ent_counter;
    }

    void write_to_build(struct entity_compile_output *output,
                        blob_v0 *build) {
        struct entity_resource res = {0};
        res.ent_count = (uint32_t) (output->ent_counter);
        res.comp_type_count = (uint32_t) array::size(output->component_type);

        build->push(build->inst, &res, sizeof(struct entity_resource));

        //write parents
        for (int i = 0; i < res.ent_count; ++i) {
            uint32_t id = map::get(output->entity_parent, i, UINT32_MAX);

            build->push(build->inst, &id, sizeof(id));
        }

        //write comp types
        build->push(build->inst,
                    (uint8_t *) array::begin(output->component_type),
                    sizeof(uint64_t) *
                    array::size(output->component_type));

        //write comp data
        for (int j = 0; j < res.comp_type_count; ++j) {
            uint64_t cid = output->component_type[j];
            uint64_t id = cid;

            uint32_t idx = map::get(output->component_ent, cid, UINT32_MAX);
            Array<uint32_t> &ent_arr = output->component_ent_array[idx];

            struct component_data cdata = {
                    .ent_count = (uint32_t) array::size(ent_arr)
            };

            idx = map::get(output->component_body, cid, UINT32_MAX);
            Array<yaml_node_t> &body = output->component_body_array[idx];

            blob_v0 *blob = blob_api_v0.create(memory_api_v0.main_allocator());

            for (int i = 0; i < cdata.ent_count; ++i) {
                component_api_v0.compile(id, body[i], blob);
            }

            cdata.size = blob->size(blob->inst);

            build->push(build->inst, (uint8_t *) &cdata, sizeof(cdata));
            build->push(build->inst, (uint8_t *) array::begin(ent_arr),
                        sizeof(uint32_t) * cdata.ent_count);

            build->push(build->inst, blob->data(blob->inst),
                        sizeof(uint8_t) * blob->size(blob->inst));

            blob_api_v0.destroy(blob);
        }
    }

    void compiler(yaml_node_t root,
                  const char *filename,
                  blob_v0 *build,
                  struct compilator_api *compilator_api) {
        entity_compile_output *output = create_output();
        compile_entity(output, root, filename, compilator_api);
        write_to_build(output, build);

        destroy_output(output);
    }

    int _entity_resource_compiler(const char *filename,
                                  struct os_vio *source_vio,
                                  struct os_vio *build_vio,
                                  struct compilator_api *compilator_api) {

        char source_data[source_vio->size(source_vio->inst) + 1];
        memset(source_data, 0, source_vio->size(source_vio->inst) + 1);
        source_vio->read(source_vio->inst, source_data, sizeof(char),
                         source_vio->size(source_vio->inst));

        yaml_document_t h;
        yaml_node_t root = yaml_load_str(source_data, &h);


        blob_v0 *entity_data = blob_api_v0.create(
                memory_api_v0.main_allocator());

        compiler(root, filename, entity_data, compilator_api);

        build_vio->write(build_vio->inst, entity_data->data(entity_data->inst),
                            sizeof(uint8_t),
                            entity_data->size(entity_data->inst));


        blob_api_v0.destroy(entity_data);
        return 1;
    }
}
#endif

//==============================================================================
// Resource
//==============================================================================

namespace entity_resorce {

    void *loader(struct os_vio *input,
                 struct allocator *allocator) {
        const int64_t size = input->size(input->inst);
        char *data = CETECH_ALLOCATE(allocator, char, size);
        input->read(input->inst, data, 1, size);

        return data;
    }

    void unloader(void *new_data,
                  struct allocator *allocator) {
        CETECH_FREE(allocator, new_data);
    }


    void online(uint64_t name,
                void *data) {
    }

    void offline(uint64_t name,
                 void *data) {
    }

    void *reloader(uint64_t name,
                   void *old_data,
                   void *new_data,
                   struct allocator *allocator) {
        offline(name, old_data);
        online(name, new_data);

        CETECH_FREE(allocator, old_data);

        return new_data;
    }

    static const resource_callbacks_t callback = {
            .loader = loader,
            .unloader = unloader,
            .online = online,
            .offline = offline,
            .reloader = reloader
    };

}

//==============================================================================
// Public interface
//==============================================================================
namespace entity {
    entity_t create() {
        return (entity_t) {.h = handler::create(_G.entity_handler)};
    }

    void entity_manager_destroy(entity_t entity) {
        handler::destroy(_G.entity_handler, entity.h);
    }

    int alive(entity_t entity) {
        return handler::alive(_G.entity_handler, entity.h);
    }

    void spawn_from_resource(world_t world,
                             void *resource,
                             entity_t **entities,
                             uint32_t *entities_count) {
        struct entity_resource *res = (entity_resource *) resource;

        entity_t *spawned = CETECH_ALLOCATE(memory_api_v0.main_allocator(),
                                            entity_t, sizeof(entity_t) * res->ent_count);

        for (int j = 0; j < res->ent_count; ++j) {
            spawned[j] = create();
        }

        entity_t root = spawned[0];

        uint32_t *parents = entity_resource_parents(res);
        uint64_t *comp_types = entity_resource_comp_types(res);

        struct component_data *comp_data = entity_resource_comp_data(res);
        for (int i = 0; i < res->comp_type_count; ++i) {
            uint64_t type = comp_types[i];

            uint32_t *c_ent = component_data_ent(comp_data);
            char *c_data = component_data_data(comp_data);

            component_api_v0.spawn(world, type, &spawned[0],
                                   c_ent, parents, comp_data->ent_count,
                                   c_data);

            comp_data = (struct component_data *) (c_data + comp_data->size);
        }

        *entities = spawned;
        *entities_count = res->ent_count;
    }

    entity_t spawn(world_t world,
                   uint64_t name) {
        void *res = resource_api_v0.get(_G.type, name);

        if (res == NULL) {
            log_api_v0.error("entity", "Could not spawn entity.");
            return (entity_t) {.h = 0};
        }

        entity_t *entities = nullptr;
        uint32_t entities_count = 0;

        spawn_from_resource(world, res, &entities, &entities_count);
        CETECH_FREE(memory_api_v0.main_allocator(), entities);
        return entities[0];
    }

    void destroy(world_t world,
                 entity_t *entity,
                 uint32_t count) {

        component_api_v0.destroy(world, entity, count);

        for (int i = 0; i < count; ++i) {
            handler::destroy(_G.entity_handler, entity[i].h);
        }
    }
}

namespace entity_module {
    static struct entity_api_v0 _api = {

            .create = entity::create,
            .destroy = entity::destroy,
            .alive = entity::alive,
            .spawn_from_resource = entity::spawn_from_resource,
            .spawn = entity::spawn,

#ifdef CETECH_CAN_COMPILE
            .compiler_create_output = entity_resource_compiler::create_output,
            .compiler_destroy_output = entity_resource_compiler::destroy_output,
            .compiler_compile_entity = entity_resource_compiler::compile_entity,
            .compiler_ent_counter = entity_resource_compiler::ent_counter,
            .compiler_write_to_build = entity_resource_compiler::write_to_build,
            .resource_compiler = entity_resource_compiler::compiler
#endif
    };

    static void _init_api(struct api_v0 *api) {
        api->register_api("entity_api_v0", &_api);

    }

    static void _init(struct api_v0 *api) {
        _init_api(api);

        CETECH_GET_API(api, memory_api_v0);
        CETECH_GET_API(api, component_api_v0);
        CETECH_GET_API(api, memory_api_v0);
        CETECH_GET_API(api, resource_api_v0);
        CETECH_GET_API(api, os_path_v0);
        CETECH_GET_API(api, os_vio_api_v0);
        CETECH_GET_API(api, hash_api_v0);
        CETECH_GET_API(api, blob_api_v0);

        _G = {0};

        _G.type = hash_api_v0.id64_from_str("entity");

//        _G.spawned_map.init(memory_api_v0.main_allocator());
//        _G.spawned_array.init(memory_api_v0.main_allocator());

        resource_api_v0.register_type(_G.type, entity_resorce::callback);

#ifdef CETECH_CAN_COMPILE
        resource_api_v0.compiler_register(_G.type,
                                          entity_resource_compiler::_entity_resource_compiler);
#endif


        _G.entity_handler.init(memory_api_v0.main_allocator());
    }

    static void _shutdown() {
//        _G.spawned_map.destroy();
//        _G.spawned_array.destroy();
        _G.entity_handler.destroy();
    }


    extern "C" void entity_load_module(struct api_v0 *api) {
        _init(api);
    }

    extern "C" void entity_unload_module(struct api_v0 *api) {
        _shutdown();
    }
}