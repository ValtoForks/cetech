#include <cetech/kernel/api_system.h>
#include <cetech/kernel/module.h>
#include <cetech/kernel/os.h>
#include <cetech/kernel/log.h>

CETECH_DECL_API(ct_log_api_v0);

#include "cpu_sdl2.h"
#include "window_sdl2.h"
#include "thread_sdl2.h"
#include "sdl2_time.h"
#include "vio_sdl2.h"

namespace machine_sdl {
    void init(struct ct_api_v0 *api);
    void shutdown();
}

static struct ct_thread_api_v0 thread_api = {
        .create = thread_create,
        .kill = thread_kill,
        .wait = thread_wait,
        .get_id = thread_get_id,
        .actual_id = thread_actual_id,
        .yield = thread_yield,
        .spin_lock = thread_spin_lock,
        .spin_unlock = thread_spin_unlock
};

static struct ct_window_api_v0 window_api = {
        .create = window_new,
        .create_from = window_new_from,
        .destroy = window_destroy,
        .set_title = window_set_title,
        .get_title = window_get_title,
        .update = window_update,
        .resize = window_resize,
        .size = window_get_size,
        .native_window_ptr = window_native_window_ptr,
        .native_display_ptr = window_native_display_ptr
};

static struct ct_cpu_api_v0 cpu_api = {
        .count = cpu_count
};



static struct ct_time_api_v0 time_api = {
        .ticks =get_ticks,
        .perf_counter =get_perf_counter,
        .perf_freq =get_perf_freq
};

static struct ct_vio_api_v0 vio_api = {
        .from_file = vio_from_file,
};


extern "C" void os_load_module(struct ct_api_v0 *api) {
    CETECH_GET_API(api, ct_log_api_v0);

    api->register_api("ct_cpu_api_v0", &cpu_api);
    api->register_api("ct_time_api_v0", &time_api);

    api->register_api("ct_thread_api_v0", &thread_api);
    api->register_api("ct_window_api_v0", &window_api);
    api->register_api("ct_vio_api_v0", &vio_api);


    machine_sdl::init(api);
}

extern "C" void os_unload_module(struct ct_api_v0 *api) {
    machine_sdl::shutdown();
}


