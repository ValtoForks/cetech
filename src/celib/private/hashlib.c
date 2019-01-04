#include <stdint.h>
#include <memory.h>
#include <celib/hashlib.h>
#include <celib/module.h>
#include <celib/api_system.h>
#include <celib/hash.inl>
#include <celib/memory.h>
#include <celib/murmur_hash.inl>
#include <celib/os.h>

#define _G hashlib_global


struct _G {
    char *str_id64;
    struct ce_hash_t id64_to_str;
    struct ce_spinlock id64_to_str_lock;

    char *str_id32;
    struct ce_hash_t id32_to_str;
} _G;


#define STRINGID64_SEED 0
#define STRINGID32_SEED 0

//==============================================================================
// Interface
//==============================================================================

uint64_t stringid64_from_string(const char *str) {
    if (!str) {
        return 0;
    }

    if ('\0' == str[0]) {
        return 0;
    }

    struct ce_alloc *alloc = ce_memory_a0->system;

    const uint32_t str_len = strlen(str);

    const uint64_t hash = ce_hash_murmur2_64(str, str_len, STRINGID64_SEED);

    ce_os_a0->thread->spin_lock(&_G.id64_to_str_lock);
    if (!ce_hash_contain(&_G.id64_to_str, hash)) {
        const uint32_t idx = ce_array_size(_G.str_id64);
        ce_array_push_n(_G.str_id64, str, str_len + 1, alloc);
        ce_hash_add(&_G.id64_to_str, hash, idx, alloc);
    }
    ce_os_a0->thread->spin_unlock(&_G.id64_to_str_lock);

    return hash;
}

uint32_t stringid32_from_string(const char *str) {
    if (!str) {
        return 0;
    }

    struct ce_alloc *alloc = ce_memory_a0->system;

    const uint32_t str_len = strlen(str);

    const uint32_t hash = ct_hash_murmur2_32(str, str_len, STRINGID32_SEED);
    if (!ce_hash_contain(&_G.id32_to_str, hash)) {
        const uint32_t idx = ce_array_size(_G.str_id32);
        ce_array_push_n(_G.str_id32, str, str_len + 1, alloc);

        ce_hash_add(&_G.id32_to_str, hash, idx, alloc);
    }

    return hash;
}

const char *str_from_id64(uint64_t key) {
    uint32_t idx = ce_hash_lookup(&_G.id64_to_str, key, UINT32_MAX);

    if (UINT32_MAX != idx) {
        return &_G.str_id64[idx];
    }

    return NULL;
}

const char *str_from_id32(uint32_t key) {
    uint32_t idx = ce_hash_lookup(&_G.id32_to_str, key, UINT32_MAX);

    if (UINT32_MAX != idx) {
        return &_G.str_id32[idx];
    }

    return NULL;
}

static struct ce_id_a0 hash_api = {
        .id64 = stringid64_from_string,
//        .id32_from_str = stringid32_from_string,

        .str_from_id32 = str_from_id32,
        .str_from_id64 = str_from_id64,
};

struct ce_id_a0 *ce_id_a0 = &hash_api;

void CE_MODULE_INITAPI(hashlib)(struct ce_api_a0 *api) {
}

void CE_MODULE_LOAD (hashlib)(struct ce_api_a0 *api,
                                  int reload) {
    CE_UNUSED(reload);

    api->register_api("ce_id_a0", &hash_api);

    _G = (struct _G) {
    };
}

void CE_MODULE_UNLOAD (hashlib)(struct ce_api_a0 *api,
                                    int reload) {
    struct ce_alloc *alloc = ce_memory_a0->system;


    ce_array_free(_G.str_id64, alloc);
    ce_array_free(_G.str_id32, alloc);

    ce_hash_free(&_G.id64_to_str, alloc);
    ce_hash_free(&_G.id32_to_str, alloc);

    CE_UNUSED(api);
}
