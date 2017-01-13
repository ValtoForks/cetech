//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <include/mpack/mpack.h>

#include "include/nanomsg/nn.h"
#include "include/nanomsg/reqrep.h"
#include "include/nanomsg/pubsub.h"
#include "include/nanomsg/pipeline.h"

#include "engine/develop/console_server.h"
#include "celib/errors/errors.h"
#include "celib/string/string.h"

#include "engine/config/cvar.h"

//==============================================================================
// Defines
//==============================================================================

#define LOG_WHERE "console_server"

#define MAX_COMMANDS 256
#define MAX_COMMAND_NAME_LEN 128

//==============================================================================
// Globals
//==============================================================================

#define _G ConsoleServerGlobals

static struct G {
    char name[MAX_COMMANDS][MAX_COMMAND_NAME_LEN];
    console_server_command_t commands[MAX_COMMANDS];

    int rpc_socket;
    int log_socket;
    int push_socket;

    cvar_t cv_rpc_addr;
    cvar_t cv_log_addr;
    cvar_t cv_push_addr;
} ConsoleServerGlobals = {0};


//==============================================================================
// Private
//==============================================================================

static console_server_command_t _find_command(const char *name) {
    for (int i = 1; i < MAX_COMMANDS; ++i) {
        if (_G.name[i][0] == '\0' || cel_strcmp(_G.name[i], name) != 0) {
            continue;
        }

        return _G.commands[i];
    }

    log_error(LOG_WHERE, "Invalid command \"%s\"", name);

    return 0;
}

static void _serve_command(const char *packet,
                           int len) {
    mpack_tree_t tree;
    mpack_tree_init(&tree, packet, (size_t) len);
    mpack_tree_parse(&tree);
    mpack_error_t errort = mpack_tree_error(&tree);

    if (errort != mpack_ok) {
        log_error(LOG_WHERE, "%s", mpack_error_to_string(errort));
        return;
    }

    mpack_node_t root = mpack_tree_root(&tree);

    // Find command
    char cmd_name[256] = {0};
    mpack_node_t name_node = mpack_node_map_cstr(root, "name");
    mpack_node_copy_cstr(name_node, cmd_name, 256);

    console_server_command_t rpc_command = _find_command(cmd_name);

    char *data;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);

    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "response");

    int ret_n = 0;
    if (rpc_command != NULL) {
        //log_debug(LOG_WHERE, "call: %s", cmd_name);
        mpack_node_t args = mpack_node_map_cstr(root, "args");
        ret_n = rpc_command(args, &writer);
    }

    if (!ret_n) {
        mpack_write_nil(&writer);
    }

    mpack_finish_map(&writer);
    CEL_ASSERT(LOG_WHERE, mpack_writer_destroy(&writer) == mpack_ok);

    int bytes = nn_send(_G.rpc_socket, data, size, 0);
    CEL_ASSERT(LOG_WHERE, (size_t) bytes == size);
    free(data);
}

//==============================================================================
// Interface
//==============================================================================

static int _cmd_ready(mpack_node_t args,
                      mpack_writer_t *writer) {
    return 0;
}

int consolesrv_init(int stage) {
    if (stage == 0) {
        _G = (struct G) {0};

        _G.cv_rpc_addr = cvar_new_str("develop.rpc.addr", "Console server rpc addr", "ws://*:4444");
        _G.cv_log_addr = cvar_new_str("develop.log.addr", "Console server log addr", "ws://*:4445");
        _G.cv_push_addr = cvar_new_str("develop.push.addr", "Push addr", "");

        return 1;
    }

    const char *addr = 0;

    log_debug(LOG_WHERE, "Init");

    int socket = nn_socket(AF_SP, NN_REP);
    if (socket < 0) {
        log_error(LOG_WHERE, "Could not create nanomsg socket: %s", nn_strerror(errno));
        return 0;
    }
    addr = cvar_get_string(_G.cv_rpc_addr);

    log_debug(LOG_WHERE, "RPC address: %s", addr);

    if (nn_bind(socket, addr) < 0) {
        log_error(LOG_WHERE, "Could not bind socket to '%s': %s", addr, nn_strerror(errno));
        return 0;
    }

    _G.rpc_socket = socket;
////

    if (cvar_get_string(_G.cv_push_addr)[0] != '\0') {
        socket = nn_socket(AF_SP, NN_PUSH);
        if (socket < 0) {
            log_error(LOG_WHERE, "Could not create nanomsg socket: %s", nn_strerror(errno));
            return 0;
        }

        addr = cvar_get_string(_G.cv_push_addr);

        log_debug(LOG_WHERE, "Push address: %s", addr);

        if (nn_connect(socket, addr) < 0) {
            log_error(LOG_WHERE, "Could not bind socket to '%s': %s", addr, nn_strerror(errno));
            return 0;
        }
        _G.push_socket = socket;
    }


////
    socket = nn_socket(AF_SP, NN_PUB);
    if (socket < 0) {
        log_error(LOG_WHERE, "Could not create nanomsg socket: %s", nn_strerror(errno));
        return 0;
    }

    addr = cvar_get_string(_G.cv_log_addr);

    log_debug(LOG_WHERE, "LOG address: %s", addr);

    if (nn_bind(socket, addr) < 0) {
        log_error(LOG_WHERE, "Could not bind socket to '%s': %s", addr, nn_strerror(errno));
        return 0;
    }
    _G.log_socket = socket;

    log_register_handler(nano_log_handler, &_G.log_socket);

    consolesrv_register_command("console_server.ready", _cmd_ready);
    return 1;
}

void consolesrv_shutdown() {
    log_debug(LOG_WHERE, "Shutdown");

    nn_close(_G.push_socket);
    //nn_close(_G.log_socket);
    nn_close(_G.rpc_socket);
}

void consolesrv_register_command(const char *name,
                                 console_server_command_t cmd) {
    for (int i = 1; i < MAX_COMMANDS; ++i) {
        if (_G.name[i][0] != '\0') {
            continue;
        }

        cel_str_set(&_G.name[i][0], name);
        _G.commands[i] = cmd;
        break;
    }
}

void consolesrv_push_begin() {
    if (_G.push_socket) {
        nn_send(_G.push_socket, "begin", strlen("begin") + 1, 0);
    }
}

void consolesrv_update() {
    char *buf = NULL;

    int max_iteration = 100;
    int bytes = 0;
    while (--max_iteration) {
        bytes = nn_recv(_G.rpc_socket, &buf, NN_MSG, NN_DONTWAIT);

        if (bytes <= 0) {
            break;
        }

        _serve_command(buf, bytes);
        nn_freemsg(buf);
    }
}
