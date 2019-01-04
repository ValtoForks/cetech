/***********************************************************************
**** Includes
***********************************************************************/

#include <stdio.h>

#include <celib/platform.h>

#if CE_PLATFORM_LINUX

#include <sys/file.h>

#endif

#include "celib/log.h"
#include <memory.h>
#include "celib/macros.h"

/***********************************************************************
**** Internals
***********************************************************************/

#define FBLACK      "\033["

#define BRED        "31m"
#define BGREEN      "32m"
#define BYELLOW     "33m"
#define BBLUE       "34m"

#define NONE        "\033[0m"

#define LOG_FORMAT   \
    "---\n"          \
    "level: %s\n"    \
    "where: %s\n"    \
    "time: %s\n"     \
    "worker: %d\n"   \
    "msg: |\n  %s\n"

#if CE_PLATFORM_LINUX || CE_PLATFORM_OSX
#define CE_COLORED_LOG 1
#endif

#ifdef CE_COLORED_LOG
#define COLORED_TEXT(color, text) FBLACK color text NONE
#else
#define COLORED_TEXT(color, text) text
#endif


static char *_time_to_str(struct tm *gmtm) {
    char *time_str = asctime(gmtm);
    time_str[strlen(time_str) - 1] = '\0';
    return time_str;
}


/***********************************************************************
**** Interface implementation
***********************************************************************/

void ct_log_stdout_yaml_handler(enum ce_log_level level,
                                time_t time,
                                char worker_id,
                                const char *where,
                                const char *msg,
                                void *data) {
    CE_UNUSED(data);

    static const char *_level_to_str[4] = {
            [LOG_INFO]    = "info",
            [LOG_WARNING] = "warning",
            [LOG_ERROR]   = "error",
            [LOG_DBG]     = "debug"
    };

    static const char *_level_format[4] = {
            [LOG_INFO]    = LOG_FORMAT,
            [LOG_WARNING] = COLORED_TEXT(BYELLOW, LOG_FORMAT),
            [LOG_ERROR]   = COLORED_TEXT(BRED, LOG_FORMAT),
            [LOG_DBG]     = COLORED_TEXT(BGREEN, LOG_FORMAT)
    };

    FILE *out = level == LOG_ERROR ? stderr : stdout;

    struct tm *gmtm = gmtime(&time);
    const char *time_str = _time_to_str(gmtm);

    fprintf(out, _level_format[level], _level_to_str[level],
            where, time_str, worker_id, msg);

}


#define LOG_FORMAT_SIMPLE   \
    "[%s|%d|%s] => %s\n"

void ct_log_stdout_handler(enum ce_log_level level,
                           time_t time,
                           char worker_id,
                           const char *where,
                           const char *msg,
                           void *data) {
    CE_UNUSED(data);

    static const char *_level_to_str[4] = {
            [LOG_INFO]    = "I",
            [LOG_WARNING] = "W",
            [LOG_ERROR]   = "E",
            [LOG_DBG]     = "D"
    };

    static const char *_level_format[4] = {
            [LOG_INFO]    = LOG_FORMAT_SIMPLE,
            [LOG_WARNING] = COLORED_TEXT(BYELLOW, LOG_FORMAT_SIMPLE),
            [LOG_ERROR]   = COLORED_TEXT(BRED, LOG_FORMAT_SIMPLE),
            [LOG_DBG]     = COLORED_TEXT(BGREEN, LOG_FORMAT_SIMPLE)
    };

    FILE *out = ((level == LOG_ERROR) || (level == LOG_WARNING)) ? stderr
                                                                 : stdout;


    fprintf(out,
            _level_format[level], _level_to_str[level],
            worker_id, where,
            msg);
}
