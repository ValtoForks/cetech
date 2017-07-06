

#include <cetech/celib/array.inl>

#include <cetech/kernel/api_system.h>
#include <cetech/kernel/module.h>
#include <cetech/modules/blob.h>


using namespace cetech;

typedef cetech::Array<uint8_t> blob_array;

namespace blob {
    uint8_t *data(ct_blob_instance_v0 *inst) {
        blob_array *array = (blob_array *) inst;
        return array::begin(*array);
    }

    uint64_t size(ct_blob_instance_v0 *inst) {
        blob_array *array = (blob_array *) inst;
        return array::size(*array);
    }

    void push(ct_blob_instance_v0 *inst,
              void *data,
              uint64_t size) {
        blob_array *array = (blob_array *) inst;
        return array::push(*array, (const uint8_t *) data, size);
    }

    ct_blob_v0 *create(ct_allocator *allocator) {
        blob_array *inst = CETECH_NEW(allocator,
                                      blob_array,
                                      allocator);

        ct_blob_v0 *blob = CETECH_NEW(allocator,
                                      ct_blob_v0);
        blob->inst = inst;
        blob->push = push;
        blob->size = size;
        blob->data = data;

        return blob;
    }

    void destroy(ct_blob_v0 *blob) {
        blob_array *inst = (blob_array *) blob->inst;
        ct_allocator *a = inst->_allocator;

        CETECH_DELETE(a, blob_array, inst);
        CETECH_DELETE(a, ct_blob_v0, blob);
    }

    static ct_blob_api_v0 api{
            .create = blob::create,
            .destroy = blob::destroy,
    };
}


namespace blob_module {
    extern "C" void blob_load_module(struct ct_api_v0 *api) {
        api->register_api("ct_blob_api_v0", &blob::api);
    }

    extern "C" void blob_unload_module(struct ct_api_v0 *api) {
    }
}