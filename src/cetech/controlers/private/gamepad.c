//==============================================================================
// Includes
//==============================================================================
#include <string.h>

#include <celib/api_system.h>
#include <celib/log.h>
#include <celib/module.h>
#include <celib/ebus.h>
#include <celib/hashlib.h>
#include <celib/macros.h>
#include <celib/os.h>
#include "celib/allocator.h"

#include <cetech/machine/machine.h>
#include <cetech/kernel/kernel.h>
#include <cetech/controlers/gamepad.h>
#include <cetech/controlers/controlers.h>
#include <celib/cdb.h>

#include "gamepadstr.h"


//==============================================================================
// Defines
//==============================================================================

#define LOG_WHERE "gamepad"


//==============================================================================
// Globals
//==============================================================================

#define GAMEPAD_MAX 8

#define _G GamepadGlobals
static struct _G {
    int active[GAMEPAD_MAX];
    float position[GAMEPAD_MAX][GAMEPAD_AXIX_MAX][2];
    int state[GAMEPAD_MAX][GAMEPAD_BTN_MAX];
    int last_state[GAMEPAD_MAX][GAMEPAD_BTN_MAX];
} _G = {};


//==============================================================================
// Interface
//==============================================================================

static int is_active(uint32_t idx) {
    return _G.active[idx];
}

static uint32_t button_index(const char *button_name) {
    for (uint32_t i = 0; i < GAMEPAD_BTN_MAX; ++i) {
        if (!_btn_to_str[i]) {
            continue;
        }

        if (strcmp(_btn_to_str[i], button_name)) {
            continue;
        }

        return i;
    }

    return 0;
}

static const char *button_name(const uint32_t button_index) {
    CE_ASSERT(LOG_WHERE,
                  (button_index >= 0) && (button_index < GAMEPAD_BTN_MAX));

    return _btn_to_str[button_index];
}

static int button_state(uint32_t idx,
                        const uint32_t button_index) {
    CE_ASSERT(LOG_WHERE,
                  (button_index >= 0) && (button_index < GAMEPAD_BTN_MAX));

    return _G.state[idx][button_index];
}

static int button_pressed(uint32_t idx,
                          const uint32_t button_index) {
    CE_ASSERT(LOG_WHERE,
                  (button_index >= 0) && (button_index < GAMEPAD_BTN_MAX));

    return _G.state[idx][button_index] && !_G.last_state[idx][button_index];
}

static int button_released(uint32_t idx,
                           const uint32_t button_index) {
    CE_ASSERT(LOG_WHERE,
                  (button_index >= 0) && (button_index < GAMEPAD_BTN_MAX));

    return !_G.state[idx][button_index] && _G.last_state[idx][button_index];
}

static const char *axis_name(const uint32_t axis_index) {
    CE_ASSERT(LOG_WHERE,
                  (axis_index >= 0) && (axis_index < GAMEPAD_AXIX_MAX));

    return _axis_to_str[axis_index];
}

static uint32_t axis_index(const char *axis_name) {
    for (uint32_t i = 0; i < GAMEPAD_AXIX_MAX; ++i) {
        if (!_axis_to_str[i]) {
            continue;
        }

        if (strcmp(_axis_to_str[i], axis_name) != 0) {
            continue;
        }

        return i;
    }

    return 0;
}

static void axis(uint32_t idx,
                 const uint32_t axis_index,
                 float *value) {
    CE_ASSERT(LOG_WHERE,
                  (axis_index >= 0) && (axis_index < GAMEPAD_AXIX_MAX));

    value[0] = _G.position[idx][axis_index][0];
    value[1] = _G.position[idx][axis_index][1];
}

static void play_rumble(uint32_t idx,
                        float strength,
                        uint32_t length) {
    ct_machine_a0->gamepad_play_rumble(idx, strength, length);
}

static void update(float dt) {
    memcpy(_G.last_state, _G.state,
           sizeof(int) * GAMEPAD_BTN_MAX * GAMEPAD_MAX);

    struct ebus_event_header *it = ce_ebus_a0->events(GAMEPAD_EBUS);
    struct ebus_event_header *end_it = ce_ebus_a0->events_end(GAMEPAD_EBUS);

    while (it != end_it) {
        struct ebus_cdb_event *obj_event = CE_EBUS_BODY(it);

        const ce_cdb_obj_o * reader = ce_cdb_a0->read(obj_event->obj);

        uint32_t button = ce_cdb_a0->read_uint64(reader, CONTROLER_BUTTON, 0);
        uint32_t axis = ce_cdb_a0->read_uint64(reader, CONTROLER_AXIS, 0);
        uint32_t gamepad_id = ce_cdb_a0->read_uint64(reader, CONTROLER_ID, 0);

        float pos[3] = {
                ce_cdb_a0->read_float(reader,
                                      CONTROLER_POSITION_X, 0.0f),
                ce_cdb_a0->read_float(reader,
                                      CONTROLER_POSITION_Y, 0.0f),
                ce_cdb_a0->read_float(reader,
                                      CONTROLER_POSITION_Z, 0.0f),
        };

        switch (it->type) {
            case EVENT_GAMEPAD_DOWN:
                _G.state[gamepad_id][button] = 1;
                break;

            case EVENT_GAMEPAD_UP:
                _G.state[gamepad_id][button] = 0;
                break;

            case EVENT_GAMEPAD_MOVE:
                _G.position[gamepad_id][axis][0] = pos[0];
                _G.position[gamepad_id][axis][1] = pos[1];
                break;

            case EVENT_GAMEPAD_CONNECT:
                _G.active[gamepad_id] = 1;
                break;

            case EVENT_GAMEPAD_DISCONNECT:
                _G.active[gamepad_id] = 0;
                break;


            default:
                break;
        }

        it = CE_EBUS_NEXT(it);
    }

}

static uint64_t name() {
    return CONTROLER_GAMEPAD;
}

static struct ct_controlers_i0 ct_controlers_i0 = {
        .name = name,
        .is_active = is_active,
        .button_index = button_index,
        .button_name = button_name,
        .button_state = button_state,
        .button_pressed = button_pressed,
        .button_released = button_released,
        .axis_index = axis_index,
        .axis_name = axis_name,
        .axis = axis,
        .play_rumble = play_rumble,
};

static uint64_t task_name() {
    return CT_GAMEPAD_TASK;
}

static uint64_t * update_before(uint64_t* n) {
    static uint64_t a[] = {
            CT_INPUT_TASK,
    };

    *n = CE_ARRAY_LEN(a);
    return a;
}

static uint64_t * update_after(uint64_t* n) {
    static uint64_t a[] = {
            CT_MACHINE_TASK,
    };

    *n = CE_ARRAY_LEN(a);
    return a;
}


static struct ct_kernel_task_i0 gamepad_task = {
        .name = task_name,
        .update = update,
        .update_before= update_before,
        .update_after = update_after,
};

static void _init_api(struct ce_api_a0 *api) {
    api->register_api("ct_controlers_i0", &ct_controlers_i0);
    api->register_api("ct_kernel_task_i0", &gamepad_task);
}

static void _init(struct ce_api_a0 *api) {
    _init_api(api);
    _G = (struct _G) {};

    ce_ebus_a0->create_ebus(GAMEPAD_EBUS);

    ce_log_a0->debug(LOG_WHERE, "Init");

    for (int i = 0; i < GAMEPAD_MAX; ++i) {
        _G.active[i] = ct_machine_a0->gamepad_is_active(i);
    }
}

static void _shutdown() {
    ce_log_a0->debug(LOG_WHERE, "Shutdown");

    _G = (struct _G) {};
}

CE_MODULE_DEF(
        gamepad,
        {
            CE_INIT_API(api, ct_machine_a0);
            CE_INIT_API(api, ce_log_a0);
            CE_INIT_API(api, ce_ebus_a0);
            CE_INIT_API(api, ce_id_a0);
            CE_INIT_API(api, ce_cdb_a0);
        },
        {
            CE_UNUSED(reload);
            _init(api);
        },
        {
            CE_UNUSED(reload);
            CE_UNUSED(api);

            _shutdown();

        }
)