//==============================================================================
// Includes
//==============================================================================


#include "celib/container_types.inl"
#include "celib/array.inl"
#include "celib/map.inl"

#include <cetech/application/application.h>
#include <cetech/api/api_system.h>
#include <cetech/os/memory.h>
#include <cetech/filesystem/filesystem.h>
#include <cetech/config/config.h>
#include <cetech/os/path.h>
#include <cetech/os/vio.h>
#include <cetech/log/log.h>
#include <cetech/os/thread.h>
#include <cetech/os/errors.h>
#include <cetech/resource/package.h>
#include <cetech/module/module.h>
#include <cetech/coredb/coredb.h>
#include <cetech/kernel/kernel.h>
#include <celib/array.h>

#include "include/SDL2/SDL.h"

#include "resource.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_filesystem_a0);
CETECH_DECL_API(ct_config_a0);
CETECH_DECL_API(ct_coredb_a0);
CETECH_DECL_API(ct_path_a0);
CETECH_DECL_API(ct_vio_a0);
CETECH_DECL_API(ct_log_a0);
CETECH_DECL_API(ct_hash_a0);
CETECH_DECL_API(ct_thread_a0);


void resource_register_type(uint64_t type,
                            ct_resource_callbacks_t callbacks);


using namespace celib;


namespace resource {
    void reload_all();
}

//==============================================================================
// Gloals
//==============================================================================

#define LOG_WHERE "resource_manager"
#define is_item_null(item) ((item).data == null_item.data)


static uint64_t hash_combine(uint64_t lhs,
                      uint64_t rhs) {
    if(lhs == 0) return rhs;
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    return lhs;
}

//==============================================================================
// Gloals
//==============================================================================

namespace {
    typedef struct {
        uint64_t type;
        uint64_t name;
        void *data;
        uint8_t ref_count;
    } resource_item_t;

    static const resource_item_t null_item = {};

#define _G ResourceManagerGlobals
    struct ResourceManagerGlobals {
        Map<uint32_t> type_map;
        ct_resource_callbacks_t* resource_callbacks;

        Map<uint32_t> resource_map;
        resource_item_t* resource_data;

        int autoload_enabled;

        ct_spinlock add_lock;
        ct_coredb_object_t* config_object;
        cel_alloc* allocator;
    } ResourceManagerGlobals = {};
}

//==============================================================================
// Private
//==============================================================================


#define CONFIG_BUILD_DIR CT_ID64_0(CONFIG_BUILD_ID)
#define CONFIG_KERNEL_PLATFORM CT_ID64_0(CONFIG_PLATFORM_ID)

char *resource_compiler_get_build_dir(cel_alloc *a,
                                      const char *platform) {

    const char *build_dir_str = ct_coredb_a0.read_string(_G.config_object, CONFIG_BUILD_DIR, "");
    return ct_path_a0.join(a, 2, build_dir_str, platform);
}

namespace package_resource {

    void *loader(ct_vio *input,
                 cel_alloc *allocator) {

        const int64_t size = input->size(input);
        char *data = CEL_ALLOCATE(allocator, char, size);
        input->read(input, data, 1, size);

        return data;
    }

    void unloader(void *new_data,
                  cel_alloc *allocator) {
        CEL_FREE(allocator, new_data);
    }

    void online(uint64_t name,
                void *data) {
        CEL_UNUSED(name, data);
    }

    void offline(uint64_t name,
                 void *data) {
        CEL_UNUSED(name, data);
    }

    void *reloader(uint64_t name,
                   void *old_data,
                   void *new_data,
                   cel_alloc *allocator) {
        CEL_UNUSED(name);

        CEL_FREE(allocator, old_data);
        return new_data;
    }

    static const ct_resource_callbacks_t package_resource_callback = {
            .loader = loader,
            .unloader =unloader,
            .online =online,
            .offline =offline,
            .reloader = reloader
    };
};

//==============================================================================
// Public interface
//==============================================================================

namespace resource {

    int type_name_string(char *str,
                         size_t max_len,
                         uint64_t type,
                         uint64_t name) {
        return snprintf(str, max_len, "%" SDL_PRIX64 "%" SDL_PRIX64, type,
                        name);
    }


    void set_autoload(int enable) {
        _G.autoload_enabled = enable;
    }

    void resource_register_type(uint64_t type,
                                ct_resource_callbacks_t callbacks) {

        const uint32_t idx = cel_array_size(_G.resource_callbacks);

        cel_array_push(_G.resource_callbacks, callbacks, _G.allocator);
        map::set(_G.type_map, type, idx);
    }

    void add_loaded(uint64_t type,
                    uint64_t *names,
                    void **resource_data,
                    size_t count) {
        ct_thread_a0.spin_lock(&_G.add_lock);


        const uint32_t type_idx = map::get(_G.type_map, type, UINT32_MAX);

        if (type_idx == UINT32_MAX) {
            ct_thread_a0.spin_unlock(&_G.add_lock);
            return;
        }


        for (size_t i = 0; i < count; i++) {

            if (resource_data[i] == 0) {
                continue;
            }

            resource_item_t item = {
                    .ref_count=1,
                    .name = names[i],
                    .type = type,
                    .data=resource_data[i]
            };

            uint64_t id = hash_combine(type, names[i]);

            if (!map::has(_G.resource_map, id)) {
                uint32_t idx = cel_array_size(_G.resource_data);
                cel_array_push(_G.resource_data, item, _G.allocator);
                map::set(_G.resource_map, id, idx);
            } else {
                uint32_t idx = map::get(_G.resource_map, id, UINT32_MAX);
                _G.resource_data[idx] = item;
            }


            _G.resource_callbacks[type_idx].online(names[i], resource_data[i]);
        }

        ct_thread_a0.spin_unlock(&_G.add_lock);
    }

    void load(void **loaded_data,
              uint64_t type,
              uint64_t *names,
              size_t count,
              int force);

    void load_now(uint64_t type,
                  uint64_t *names,
                  size_t count) {
        void *loaded_data[count];

        load(loaded_data, type, names, count, 0);
        add_loaded(type, names, loaded_data, count);
    }

    int can_get(uint64_t type,
                uint64_t name) {

        if (!map::has(_G.type_map, type)) {
            return 1;
        }

        ct_thread_a0.spin_lock(&_G.add_lock);

        uint64_t id = hash_combine(type, name);
        int h = map::has(_G.resource_map, id);

        ct_thread_a0.spin_unlock(&_G.add_lock);

        return h;
    }

    int can_get_all(uint64_t type,
                    uint64_t *names,
                    size_t count) {

//        ct_thread_a0.spin_lock(&_G.add_lock);

        for (size_t i = 0; i < count; ++i) {
            if (!can_get(type, names[i])) {
                //ce_thread_a0.spin_unlock(&_G.add_lock);
                return 0;
            }
        }

//        ct_thread_a0.spin_unlock(&_G.add_lock);

        return 1;
    }

    void load(void **loaded_data,
              uint64_t type,
              uint64_t *names,
              size_t count,
              int force) {

        ct_thread_a0.spin_lock(&_G.add_lock);

        const uint32_t idx = map::get(_G.type_map, type, UINT32_MAX);

        if (idx == UINT32_MAX) {
            ct_log_a0.error(LOG_WHERE,
                            "Loader for resource is not is not registred");
            memset(loaded_data, sizeof(void *), count);
            ct_thread_a0.spin_unlock(&_G.add_lock);
            return;
        }

        const uint64_t root_name = CT_ID64_0("build");
        ct_resource_callbacks_t type_clb = _G.resource_callbacks[idx];


        for (uint32_t i = 0; i < count; ++i) {
            uint64_t id = hash_combine(type, names[i]);
            uint32_t res_idx = map::get(_G.resource_map, id, UINT32_MAX);
            resource_item_t item = {};
            if (res_idx != UINT32_MAX) {
                item = _G.resource_data[idx];
            }

            if (!force && (item.ref_count > 0)) {
                ++item.ref_count;
                _G.resource_data[res_idx] = item;
                loaded_data[i] = 0;
                continue;
            }

            char build_name[33] = {};
            type_name_string(build_name, CETECH_ARRAY_LEN(build_name),
                             type,
                             names[i]);

            char filename[1024] = {};
            resource_compiler_get_filename(filename, CETECH_ARRAY_LEN(filename),
                                           type,
                                           names[i]);
//#else
//            char *filename = build_name;
//#endif
            ct_log_a0.debug("resource", "Loading resource %s from %s",
                            filename,
                            build_name);

            char *build_full = ct_path_a0.join(
                    ct_memory_a0.main_allocator(), 2,
                    ct_coredb_a0.read_string(_G.config_object, CONFIG_KERNEL_PLATFORM, ""),
                    build_name);

            ct_vio *resource_file = ct_filesystem_a0.open(root_name,
                                                          build_full,
                                                          FS_OPEN_READ);
            CEL_FREE(ct_memory_a0.main_allocator(), build_full);

            if (resource_file != NULL) {
                loaded_data[i] = type_clb.loader(resource_file,
                                                 ct_memory_a0.main_allocator());
                ct_filesystem_a0.close(resource_file);
            } else {
                loaded_data[i] = 0;
            }
        }

        ct_thread_a0.spin_unlock(&_G.add_lock);
    }

    void unload(uint64_t type,
                uint64_t *names,
                size_t count) {
        ct_thread_a0.spin_lock(&_G.add_lock);

        const uint32_t idx = map::get(_G.type_map, type, UINT32_MAX);

        if (idx == UINT32_MAX) {
            ct_thread_a0.spin_unlock(&_G.add_lock);
            return;
        }

        ct_resource_callbacks_t type_clb = _G.resource_callbacks[idx];

        for (uint32_t i = 0; i < count; ++i) {
            uint64_t id = hash_combine(type, names[i]);
            uint32_t res_idx = map::get(_G.resource_map, id, UINT32_MAX);

            if (res_idx == UINT32_MAX) {
                continue;
            }

            resource_item_t &item = _G.resource_data[res_idx];

            if (item.ref_count == 0) {
                continue;
            }

            if (--item.ref_count == 0) {
                char build_name[33] = {};
                type_name_string(build_name,
                                 CETECH_ARRAY_LEN(build_name),
                                 type, names[i]);

                char filename[1024] = {};
                resource_compiler_get_filename(filename,
                                               CETECH_ARRAY_LEN(filename),
                                               type,
                                               names[i]);
//#else
//                char *filename = build_name;
//#endif

                ct_log_a0.debug("resource", "Unload resource %s ", filename);

                type_clb.offline(names[i], item.data);
                type_clb.unloader(item.data, ct_memory_a0.main_allocator());

                map::remove(_G.resource_map, hash_combine(type, names[i]));
            }

            //_G.resource_data[idx] = item;
        }
        ct_thread_a0.spin_unlock(&_G.add_lock);
    }

    void *get(uint64_t type,
              uint64_t name) {
        //ce_thread_a0.spin_lock(&_G.add_lock);

        uint64_t id = hash_combine(type, name);
        uint32_t idx = map::get(_G.resource_map, id, UINT32_MAX);
        resource_item_t item = {};
        if (idx != UINT32_MAX) {
            item = _G.resource_data[idx];
        }

        if (is_item_null(item)) {
            char build_name[33] = {};
            type_name_string(build_name, CETECH_ARRAY_LEN(build_name),
                             type,
                             name);

            if (_G.autoload_enabled) {
                char filename[1024] = {};
                resource_compiler_get_filename(filename,
                                               CETECH_ARRAY_LEN(filename),
                                               type,
                                               name);
//#else
//                char *filename = build_name;
//#endif
                ct_log_a0.warning(LOG_WHERE, "Autoloading resource %s",
                                  filename);
                load_now(type, &name, 1);

                uint64_t type_name = hash_combine(type, name);
                uint32_t res_idx = map::get(_G.resource_map, type_name,
                                            UINT32_MAX);
                item = {};
                if (res_idx != UINT32_MAX) {
                    item = _G.resource_data[res_idx];
                }

            } else {
                // TODO: fallback resource #205
                CETECH_ASSERT(LOG_WHERE, false);
            }
        }

        //ce_thread_a0.spin_unlock(&_G.add_lock);

        return item.data;
    }

    void reload(uint64_t type,
                uint64_t *names,
                size_t count) {
//        reload_all();

        void *loaded_data[count];

        const uint32_t idx = map::get<uint32_t>(_G.type_map, type, 0);

        ct_resource_callbacks_t type_clb = _G.resource_callbacks[idx];

        load(loaded_data, type, names, count, 1);
        for (uint32_t i = 0; i < count; ++i) {

            char filename[1024] = {};
            resource_compiler_get_filename(filename, CETECH_ARRAY_LEN(filename),
                                           type,
                                           names[i]);
//#else
//            char build_name[33] = {};
//            resource::type_name_string(build_name, CETECH_ARRAY_LEN(build_name),
//                                       type, names[i]);
//
//            char *filename = build_name;
//#endif
            ct_log_a0.debug("resource", "Reload resource %s ", filename);

            void *old_data = get(type, names[i]);

            void *new_data = type_clb.reloader(names[i], old_data,
                                               loaded_data[i],
                                               ct_memory_a0.main_allocator());

            uint64_t id = hash_combine(type, names[i]);
            uint32_t item_idx = map::get(_G.resource_map, id, UINT32_MAX);
            if (item_idx == UINT32_MAX) {
                continue;
            }

            resource_item_t item = _G.resource_data[item_idx];
            item.data = new_data;
            //--item.ref_count; // Load call increase item.ref_count, because is loaded
            _G.resource_data[item_idx] = item;
        }
    }

    void reload_all() {
        const Map<uint32_t>::Entry *type_it = map::begin(_G.type_map);
        const Map<uint32_t>::Entry *type_end = map::end(_G.type_map);

        uint64_t* name_array = NULL;

        while (type_it != type_end) {
            uint64_t type_id = type_it->key;

            cel_array_clean(name_array);

            for (uint32_t i = 0; i < cel_array_size(_G.resource_data); ++i) {
                resource_item_t item = _G.resource_data[i];

                if (item.type == type_id) {
                    cel_array_push(name_array, item.name, _G.allocator);
                }
            }

            reload(type_id, &name_array[0], cel_array_size(name_array));

            ++type_it;
        }
    }

}

    void resource_memory_reload(uint64_t type, uint64_t name, char** blob) {
        const uint32_t idx = map::get<uint32_t>(_G.type_map, type, UINT32_MAX);

        const uint64_t id = hash_combine(type, name);
        const uint32_t item_idx = map::get(_G.resource_map, id, UINT32_MAX);
        if (item_idx == UINT32_MAX) {
            return;
        }

        ct_resource_callbacks_t type_clb = _G.resource_callbacks[idx];

        void *old_data = resource::get(type, name);

        void *new_data = type_clb.reloader(name, old_data,
                                           blob,
                                           ct_memory_a0.main_allocator());

        resource_item_t item = _G.resource_data[item_idx];
        item.data = new_data;
        _G.resource_data[item_idx] = item;
    }

namespace resource_module {
    static ct_resource_a0 resource_api = {
            .set_autoload = resource::set_autoload,
            .register_type = resource::resource_register_type,
            .load = resource::load,
            .add_loaded = resource::add_loaded,
            .load_now = resource::load_now,
            .unload = resource::unload,
            .reload = resource::reload,
            .reload_all = resource::reload_all,
            .can_get = resource::can_get,
            .can_get_all = resource::can_get_all,
            .get = resource::get,
            .type_name_string = resource::type_name_string,

            .compiler_get_build_dir = ::resource_compiler_get_build_dir,

            .compile_and_reload = compile_and_reload,
            .compiler_get_core_dir = resource_compiler_get_core_dir,
            .compiler_register = resource_compiler_register,
            .compiler_compile_all = resource_compiler_compile_all,
            .compiler_get_filename = resource_compiler_get_filename,
            .compiler_get_tmp_dir = resource_compiler_get_tmp_dir,
            .compiler_external_join = resource_compiler_external_join,
            .compiler_create_build_dir = resource_compiler_create_build_dir,
            .compiler_get_source_dir = resource_compiler_get_source_dir,
            .type_name_from_filename = type_name_from_filename,
            .compiler_check_fs = resource_compiler_check_fs,

    };

    static ct_package_a0 package_api = {
            .load = package_load,
            .unload = package_unload,
            .is_loaded = package_is_loaded,
            .flush = package_flush,
    };


    void _init_api(ct_api_a0 *api) {
        api->register_api("ct_resource_a0", &resource_api);
        api->register_api("ct_package_a0", &package_api);
    }


    void _init_cvar(struct ct_config_a0 config) {
        _G = {};

        ct_config_a0 = config;
        _G.config_object = ct_config_a0.config_object();

        ct_coredb_writer_t* writer = ct_coredb_a0.write_begin(_G.config_object);
        if(!ct_coredb_a0.prop_exist(_G.config_object, CONFIG_BUILD_DIR)) {
            ct_coredb_a0.set_string(writer, CONFIG_BUILD_DIR, "build");
        }
        ct_coredb_a0.write_commit(writer);
    }


    void _init(ct_api_a0 *api) {
        _init_api(api);
        _init_cvar(ct_config_a0);

        _G = {
                .allocator = ct_memory_a0.main_allocator(),
                .config_object = ct_config_a0.config_object(),
        };

        _G.type_map.init(ct_memory_a0.main_allocator());
        _G.resource_map.init(ct_memory_a0.main_allocator());

        ct_filesystem_a0.map_root_dir(CT_ID64_0("build"),
                                      ct_coredb_a0.read_string(_G.config_object, CONFIG_BUILD_DIR, ""), false);

        resource::resource_register_type(CT_ID64_0("package"),
                                         package_resource::package_resource_callback);

    }

    void _shutdown() {
        package_shutdown();

        _G.type_map.destroy();
        cel_array_free(_G.resource_data, _G.allocator);
        cel_array_free(_G.resource_callbacks, _G.allocator);
        _G.resource_map.destroy();
    }

}



CETECH_MODULE_DEF(
        resourcesystem,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_filesystem_a0);
            CETECH_GET_API(api, ct_config_a0);
            CETECH_GET_API(api, ct_path_a0);
            CETECH_GET_API(api, ct_vio_a0);
            CETECH_GET_API(api, ct_log_a0);
            CETECH_GET_API(api, ct_hash_a0);
            CETECH_GET_API(api, ct_thread_a0);
            CETECH_GET_API(api, ct_coredb_a0);
        },
        {
            CEL_UNUSED(reload);
            resource_module::_init(api);
        },
        {
            CEL_UNUSED(reload);
            CEL_UNUSED(api);

            resource_module::_shutdown();

        }
)