/***********************************************************************
**** Includes
***********************************************************************/

#include <stdio.h>
#include <include/mpack/mpack.h>
#include <include/nanomsg/nn.h>

#include "celib/log/log.h"
#include "celib/errors/errors.h"

/***********************************************************************
**** Internals
***********************************************************************/

static const char *_level_to_str[4] = {
        [LOG_INFO] = "info",
        [LOG_WARNING] = "warning",
        [LOG_ERROR] = "error",
        [LOG_DBG] = "debug"
};

void nano_log_handler(enum log_level level,
                      time_t time,
                      char worker_id,
                      const char *where,
                      const char *msg,
                      void *_data) {

    CE_STATIC_ASSERT_MSG(sizeof(void *) >= sizeof(int), "AUTHOR IS IDIOT");

    int socket = *((int *) _data); // TODO: problem if sizeof(void*) < int

    int bytes;
    char *data;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);

    mpack_start_map(&writer, 5);

    mpack_write_cstr(&writer, "level");
    mpack_write_cstr(&writer, _level_to_str[level]);

    mpack_write_cstr(&writer, "time");
    mpack_write_i64(&writer, time);

    mpack_write_cstr(&writer, "worker_id");
    mpack_write_i8(&writer, worker_id);

    mpack_write_cstr(&writer, "where");
    mpack_write_cstr(&writer, where);

    mpack_write_cstr(&writer, "msg");
    mpack_write_cstr(&writer, msg);

    mpack_finish_map(&writer);

    CE_ASSERT("console_server", mpack_writer_destroy(&writer) == mpack_ok);

    bytes = nn_send(socket, data, size, 0);
    CE_ASSERT("console_server", (size_t) bytes == size);
}
