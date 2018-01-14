#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <cetech/api/api_system.h>
#include <cetech/os/memory.h>
#include <cetech/coredb/coredb.h>
#include <cetech/module/module.h>
#include <celib/macros.h>
#include <celib/allocator.h>

CETECH_DECL_API(ct_memory_a0);

#define _G coredb_global
#define LOG_WHERE "coredb"

#define MAX_OBJECTS 1000000000ULL


struct object {
    uint8_t *buffer;

    uint64_t *keys;
    enum ct_coredb_prop_type *type;
    uint64_t *offset;
    uint8_t *values;

    uint64_t buffer_size;
    uint64_t properties_count;
    uint64_t values_size;
};


static struct _G {
    struct object **objects;
    uint64_t object_used;
} _G;


static uint64_t _object_new_property(struct object *obj,
                                     uint64_t key,
                                     enum ct_coredb_prop_type type,
                                     const void *value,
                                     size_t size,
                                     const struct cel_alloc *alloc) {

    const uint64_t prop_count = obj->properties_count;
    const uint64_t values_size = obj->values_size;

    const uint64_t new_prop_count = prop_count + 1;
    const uint64_t new_value_size = values_size + size;

    const size_t new_buffer_size = ((sizeof(uint64_t) +
                                     sizeof(enum ct_coredb_prop_type) +
                                     sizeof(uint64_t)) * new_prop_count) +
                                   (sizeof(uint8_t) * new_value_size);

    struct object new_obj = {0};

    new_obj.buffer = CEL_ALLOCATE(alloc, uint8_t, new_buffer_size);
    new_obj.buffer_size = new_buffer_size;
    new_obj.properties_count = new_prop_count;
    new_obj.values_size = new_value_size;

    new_obj.keys = (uint64_t *) new_obj.buffer;
    new_obj.type = (enum ct_coredb_prop_type *) (new_obj.keys + new_prop_count);
    new_obj.offset = (uint64_t *) (new_obj.type + new_prop_count);
    new_obj.values = (uint8_t *) (new_obj.offset + new_prop_count);

    memcpy(new_obj.keys, obj->keys, sizeof(uint64_t) * prop_count);
    memcpy(new_obj.type, obj->type,
           sizeof(enum ct_coredb_prop_type) * prop_count);
    memcpy(new_obj.offset, obj->offset, sizeof(uint64_t) * prop_count);
    memcpy(new_obj.values, obj->values, sizeof(uint8_t) * values_size);

    new_obj.keys[prop_count] = key;
    new_obj.offset[prop_count] = values_size;
    new_obj.type[prop_count] = type;

    memcpy(new_obj.values + values_size, value, size);

    CEL_FREE(alloc, obj->buffer);

    *obj = new_obj;

    return prop_count;
}

struct object *_new_object(const struct cel_alloc *a) {
    struct object *obj = CEL_ALLOCATE(a, struct object, sizeof(struct object));
    *obj = (struct object) {0};

    _object_new_property(obj, 0, COREDB_TYPE_NONE, NULL, 0, a);

    return obj;
}

static struct object *_object_clone(struct object *obj,
                                    const struct cel_alloc *alloc) {
    const uint64_t properties_count = obj->properties_count;
    const uint64_t values_size = obj->values_size;
    const uint64_t buffer_size = obj->buffer_size;

    struct object *new_obj = _new_object(alloc);

    new_obj->buffer = CEL_ALLOCATE(alloc, uint8_t, buffer_size);
    new_obj->buffer_size = buffer_size;
    new_obj->properties_count = properties_count;
    new_obj->values_size = values_size;

    new_obj->keys = (uint64_t *) new_obj->buffer;
    new_obj->type = (enum ct_coredb_prop_type *) (new_obj->keys +
                                                  properties_count);
    new_obj->offset = (uint64_t *) (new_obj->type + properties_count);
    new_obj->values = (uint8_t *) (new_obj->offset + properties_count);

    memcpy(new_obj->buffer, obj->buffer, sizeof(uint8_t) * buffer_size);

    return new_obj;
}

static uint64_t _find_prop_index(uint64_t key,
                                 const uint64_t *keys,
                                 size_t size) {
    for (uint64_t i = 1; i < size; ++i) {
        if (keys[i] != key) {
            continue;
        }

        return i;
    }

    return 0;
}

static struct ct_coredb_object_t *create_object() {
    struct cel_alloc *a = ct_memory_a0.main_allocator();

    struct object *obj = _new_object(a);

    struct object **obj_addr = _G.objects + _G.object_used++;
    *obj_addr = obj;

    return (struct ct_coredb_object_t *) obj_addr;
}


struct writer_t {
    struct object **obj;
    struct object *clone_obj;
};

static struct ct_coredb_writer_t *write_begin(struct ct_coredb_object_t *obj) {
    struct writer_t *writer = CEL_ALLOCATE(ct_memory_a0.main_allocator(),
                                           struct writer_t,
                                           sizeof(struct writer_t));

    writer->clone_obj = _object_clone(*(struct object **) (obj),
                                      ct_memory_a0.main_allocator());
    writer->obj = (struct object **) obj;

    return (struct ct_coredb_writer_t *) writer;
}

static void write_commit(struct ct_coredb_writer_t *_writer) {
    struct writer_t *writer = (struct writer_t *) _writer;

    *writer->obj = writer->clone_obj;
}

static void set_float(struct ct_coredb_writer_t *_writer,
                      uint64_t property,
                      float value) {
    struct writer_t *writer = (struct writer_t *) _writer;

    struct object *obj = writer->clone_obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    if (!idx) {
        idx = _object_new_property(obj, property, COREDB_TYPE_FLOAT, &value,
                                   sizeof(float),
                                   ct_memory_a0.main_allocator());
    }

    *(float *) (obj->values + obj->offset[idx]) = value;
}

static void set_string(struct ct_coredb_writer_t *_writer,
                       uint64_t property,
                       const char *value) {
    struct cel_alloc *a = ct_memory_a0.main_allocator();

    struct writer_t *writer = (struct writer_t *) _writer;

    struct object *obj = writer->clone_obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    if (!idx) {
        idx = _object_new_property(obj, property,
                                   COREDB_TYPE_STRPTR,
                                   &value, sizeof(char *),
                                   ct_memory_a0.main_allocator());
    } else {
        CEL_FREE(a, *((char **) (obj->values + obj->offset[idx])));
    }

    char *value_clone = ct_memory_a0.str_dup(value, a);

    memcpy(obj->values + obj->offset[idx], &value_clone, sizeof(value_clone));
}

static void set_uint32(struct ct_coredb_writer_t *_writer,
                       uint64_t property,
                       uint32_t value) {
    struct writer_t *writer = (struct writer_t *) _writer;

    struct object *obj = writer->clone_obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    if (!idx) {
        idx = _object_new_property(obj, property, COREDB_TYPE_UINT32, &value,
                                   sizeof(uint32_t),
                                   ct_memory_a0.main_allocator());
    }

    *(uint32_t *) (obj->values + obj->offset[idx]) = value;
}

static void set_ptr(struct ct_coredb_writer_t *_writer,
                    uint64_t property,
                    void *value) {
    struct writer_t *writer = (struct writer_t *) _writer;

    struct object *obj = writer->clone_obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    if (!idx) {
        idx = _object_new_property(obj, property, COREDB_TYPE_PTR, &value,
                                   sizeof(uint32_t),
                                   ct_memory_a0.main_allocator());
    }

    *(void **) (obj->values + obj->offset[idx]) = value;
}


static bool prop_exist(struct ct_coredb_object_t *_object,
                       uint64_t key) {
    struct object *obj = *(struct object **) _object;

    return _find_prop_index(key, obj->keys, obj->properties_count) > 0;
}

static enum ct_coredb_prop_type prop_type(struct ct_coredb_object_t *_object,
                                          uint64_t key) {
    struct object *obj = *(struct object **) _object;

    uint64_t idx = _find_prop_index(key, obj->keys, obj->properties_count);

    return idx ? obj->type[idx] : COREDB_TYPE_NONE;
}

static float read_float(struct ct_coredb_object_t *_obj,
                        uint64_t property,
                        float defaultt) {
    struct object *obj = *(struct object **) _obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    return idx ? *(float *) (obj->values + obj->offset[idx]) : defaultt;
}

static const char *read_string(struct ct_coredb_object_t *_obj,
                               uint64_t property,
                               const char *defaultt) {
    struct object *obj = *(struct object **) _obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    return idx ? *(const char **) (obj->values + obj->offset[idx]) : defaultt;
}

static uint32_t read_uint32(struct ct_coredb_object_t *_obj,
                            uint64_t property,
                            uint32_t defaultt) {
    struct object *obj = *(struct object **) _obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    return idx ? *(uint32_t *) (obj->values + obj->offset[idx]) : defaultt;
}

static void *read_ptr(struct ct_coredb_object_t *_obj,
                      uint64_t property,
                      void *defaultt) {
    struct object *obj = *(struct object **) _obj;

    uint64_t idx = _find_prop_index(property, obj->keys, obj->properties_count);
    return idx ? *(void **) (obj->values + obj->offset[idx]) : defaultt;
}

static struct ct_coredb_a0 coredb_api = {
        .create_object = create_object,

        .prop_exist = prop_exist,
        .prop_type = prop_type,

        .read_float = read_float,
        .read_string = read_string,
        .read_uint32 = read_uint32,
        .read_ptr = read_ptr,

        .write_begin = write_begin,
        .write_commit = write_commit,

        .set_float = set_float,
        .set_string = set_string,
        .set_uint32 = set_uint32,
        .set_ptr = set_ptr,
};


static void _init(struct ct_api_a0 *api) {
    _G = (struct _G) {0};

    _G.objects = (struct object **) mmap(NULL,
                                         MAX_OBJECTS * sizeof(struct object *),
                                         PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS,
                                         -1, 0);

    api->register_api("ct_coredb_a0", &coredb_api);
}

static void _shutdown() {
    _G = (struct _G) {0};
}

CETECH_MODULE_DEF(
        coredb,
        {
            CETECH_GET_API(api, ct_memory_a0);
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