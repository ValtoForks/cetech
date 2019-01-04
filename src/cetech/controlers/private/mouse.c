//==============================================================================
// Includes
//==============================================================================
#include <string.h>
#include <celib/hashlib.h>
#include <celib/ebus.h>
#include <celib/os.h>
#include <celib/macros.h>
#include "celib/allocator.h"
#include "celib/log.h"
#include "celib/module.h"
#include "celib/api_system.h"

#include <cetech/machine/machine.h>
#include <cetech/controlers/mouse.h>
#include <cetech/kernel/kernel.h>
#include <cetech/controlers/controlers.h>
#include <celib/cdb.h>

#include "mousebtnstr.h"

//==============================================================================
// Defines
//==============================================================================

#define LOG_WHERE "mouse"


//==============================================================================
// Globals
//==============================================================================


#define _G MouseGlobals
static struct _G {
    uint8_t state[MOUSE_BTN_MAX];
    uint8_t last_state[MOUSE_BTN_MAX];
    float pos[3];
    float delta_pos[3];
    float wheel[3];
    float wheel_last[3];
} _G = {};

//==============================================================================
// Interface
//==============================================================================
static uint32_t button_index(const char *button_name) {
    for (uint32_t i = 0; i < MOUSE_BTN_MAX; ++i) {
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
              (button_index >= 0) && (button_index < MOUSE_BTN_MAX));

    return _btn_to_str[button_index];
}

static int button_state(uint32_t idx,
                        const uint32_t button_index) {
    CE_UNUSED(idx);
    CE_ASSERT(LOG_WHERE,
              (button_index >= 0) && (button_index < MOUSE_BTN_MAX));

    return _G.state[button_index];
}

static int button_pressed(uint32_t idx,
                          const uint32_t button_index) {
    CE_UNUSED(idx);
    CE_ASSERT(LOG_WHERE,
              (button_index >= 0) && (button_index < MOUSE_BTN_MAX));

    return _G.state[button_index] && !_G.last_state[button_index];
}

static int button_released(uint32_t idx,
                           const uint32_t button_index) {
    CE_UNUSED(idx);
    CE_ASSERT(LOG_WHERE,
              (button_index >= 0) && (button_index < MOUSE_BTN_MAX));

    return !_G.state[button_index] && _G.last_state[button_index];
}

static const char *axis_name(const uint32_t axis_index) {
    CE_ASSERT(LOG_WHERE,
              (axis_index >= 0) && (axis_index < MOUSE_AXIS_MAX));

    return _axis_to_str[axis_index];
}

static uint32_t axis_index(const char *axis_name) {
    for (uint32_t i = 0; i < MOUSE_AXIS_MAX; ++i) {
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
    CE_UNUSED(idx);
    CE_ASSERT(LOG_WHERE,
              (axis_index >= 0) && (axis_index < MOUSE_AXIS_MAX));

    switch (axis_index) {
        case MOUSE_AXIS_ABSOULTE:
            value[0] = _G.pos[0];
            value[1] = _G.pos[1];
            return;

        case MOUSE_AXIS_RELATIVE:
            value[0] = _G.delta_pos[0];
            value[1] = _G.delta_pos[1];

            return;

        case MOUSE_AXIS_WHEEL:
            value[0] = _G.wheel[0];
            value[1] = _G.wheel[1];

            return;

        default:
            value[0] = 0.0f;
            value[1] = 0.0f;
            return;
    }
}

//    void mouse_set_cursor_pos(vec2f_t pos) {
//        //TODO: implement
//    }

static void update(float dt) {
    memcpy(_G.last_state, _G.state, MOUSE_BTN_MAX);
    _G.delta_pos[0] = 0;
    _G.delta_pos[1] = 0;
//    _G.wheel[0] = 0;
//    _G.wheel[1] = 0;


    struct ebus_event_header *it = ce_ebus_a0->events(MOUSE_EBUS);
    struct ebus_event_header *end_it = ce_ebus_a0->events_end(MOUSE_EBUS);

    while (it != end_it) {
        struct ebus_cdb_event *obj_event = CE_EBUS_BODY(it);
        const ce_cdb_obj_o * reader = ce_cdb_a0->read(obj_event->obj);

        uint32_t button = ce_cdb_a0->read_uint64(reader,
                                                 CONTROLER_BUTTON, 0);

        switch (it->type) {
            case EVENT_MOUSE_DOWN:
                _G.state[button] = 1;
                break;

            case EVENT_MOUSE_UP:
                _G.state[button] = 0;
                break;

            case EVENT_MOUSE_MOVE: {
                float pos[3] = {
                        ce_cdb_a0->read_float(reader,
                                              CONTROLER_POSITION_X, 0.0f),
                        ce_cdb_a0->read_float(reader,
                                              CONTROLER_POSITION_Y, 0.0f),
                        ce_cdb_a0->read_float(reader,
                                              CONTROLER_POSITION_Z, 0.0f),
                };


                _G.delta_pos[0] = pos[0] - _G.pos[0];
                _G.delta_pos[1] = pos[1] - _G.pos[1];

                _G.pos[0] = pos[0];
                _G.pos[1] = pos[1];
            }
                break;

            case EVENT_MOUSE_WHEEL: {
                float pos[3] = {
                        ce_cdb_a0->read_float(reader,
                                              CONTROLER_POSITION_X, 0.0f),
                        ce_cdb_a0->read_float(reader,
                                              CONTROLER_POSITION_Y, 0.0f),
                        ce_cdb_a0->read_float(reader,
                                              CONTROLER_POSITION_Z, 0.0f),
                };

                _G.wheel[0] += pos[0];// - _G.wheel_last[0];
                _G.wheel[1] += pos[1];// - _G.wheel_last[1];

                _G.wheel_last[0] = pos[0];
                _G.wheel_last[1] = pos[1];
            }
                break;

            default:
                break;
        }

        it = CE_EBUS_NEXT(it);
    }

}

static uint64_t name() {
    return CONTROLER_MOUSE;
}

static struct ct_controlers_i0 ct_controlers_i0 = {
        .name = name,
        .button_index = button_index,
        .button_name = button_name,
        .button_state = button_state,
        .button_pressed = button_pressed,
        .button_released = button_released,
        .axis_index = axis_index,
        .axis_name = axis_name,
        .axis = axis,
};


static uint64_t task_name() {
    return CT_MOUSE_TASK;
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


static struct ct_kernel_task_i0 mouse_task = {
        .name = task_name,
        .update = update,
        .update_before= update_before,
        .update_after = update_after,
};

static void _init_api(struct ce_api_a0 *api) {
    api->register_api("ct_controlers_i0", &ct_controlers_i0);
    api->register_api("ct_kernel_task_i0", &mouse_task);
}

static void _init(struct ce_api_a0 *api) {
    _init_api(api);

    _G = (struct _G) {};

    ce_ebus_a0->create_ebus(MOUSE_EBUS);

    ce_log_a0->debug(LOG_WHERE, "Init");
}

static void _shutdown() {
    ce_log_a0->debug(LOG_WHERE, "Shutdown");

    _G = (struct _G) {};
}

CE_MODULE_DEF(
        mouse,
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