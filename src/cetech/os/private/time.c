#include <cetech/os/time.h>
#include <cetech/module/module.h>
#include <cetech/api/api_system.h>
#include "include/SDL2/SDL.h"
#include "celib/macros.h"

//==============================================================================
// Time
// ==============================================================================

uint32_t get_ticks() {
    return SDL_GetTicks();
}

uint64_t get_perf_counter() {
    return SDL_GetPerformanceCounter();
}

uint64_t get_perf_freq() {
    return SDL_GetPerformanceFrequency();
}

static struct ct_time_a0 time_api = {
        .ticks =get_ticks,
        .perf_counter =get_perf_counter,
        .perf_freq =get_perf_freq
};


CETECH_MODULE_DEF(
        time,
        {
            CEL_UNUSED(api);
        },
        {
            CEL_UNUSED(reload);
            api->register_api("ct_time_a0", &time_api);
        },
        {
            CEL_UNUSED(reload);
            CEL_UNUSED(api);
        }
)