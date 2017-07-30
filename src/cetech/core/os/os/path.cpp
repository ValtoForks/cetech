#include <dlfcn.h>
#include <errno.h>

#if defined(CETECH_LINUX)

#include <dirent.h>
#include <sys/stat.h>
#include <cetech/core/api/api_system.h>
#include <cetech/core/log/log.h>
#include <malloc.h>
#include <cetech/core/os/vio.h>
#include <cetech/core/os/path.h>
#include <cetech/core/module/module.h>

#endif

#include "celib/buffer.inl"

CETECH_DECL_API(ct_log_a0);
CETECH_DECL_API(ct_vio_a0);

#if defined(CETECH_LINUX)
#define DIR_DELIM_CH '/'
#define DIR_DELIM_STR "/"
#endif

using namespace celib;
using namespace buffer;

//==============================================================================
// File Interface
//==============================================================================

//! Get file modified time
//! \param path File path
//! \return Modified time
uint32_t file_mtime(const char *path) {
    struct stat st;
    stat(path, &st);
    return st.st_mtime;
}

//==============================================================================
// Path Interface
//==============================================================================

void _dir_list(const char *path,
               int recursive,
               Array<char *> &tmp_files,
               struct cel_alloc *allocator) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(path))) {
        return;
    }

    if (!(entry = readdir(dir))) {
        closedir(dir);
        return;
    }

    do {
        if (recursive && (entry->d_type == 4)) {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char tmp_path[1024] = {};
            int len = 0;

            if (path[strlen(path) - 1] != '/') {
                len = snprintf(tmp_path, sizeof(tmp_path) - 1, "%s/%s/", path,
                               entry->d_name);
            } else {
                len = snprintf(tmp_path, sizeof(tmp_path) - 1, "%s%s/", path,
                               entry->d_name);
            }

            _dir_list(tmp_path, 1, tmp_files, allocator);
        } else {
            size_t size = strlen(path) + strlen(entry->d_name) + 3;
            char *new_path =
                    CEL_ALLOCATE(allocator, char,
                                 sizeof(char) * size);

            if (path[strlen(path) - 1] != '/') {
                snprintf(new_path, size - 1, "%s/%s", path, entry->d_name);
            } else {
                snprintf(new_path, size - 1, "%s%s", path, entry->d_name);
            }

            array::push_back(tmp_files, new_path);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
}

void dir_list(const char *path,
              int recursive,
              char ***files,
              uint32_t *count,
              struct cel_alloc *allocator) {
    Array<char *> tmp_files(allocator);

    _dir_list(path, recursive, tmp_files, allocator);

    char **new_files = CEL_ALLOCATE(allocator, char*,
                                    sizeof(char *) * array::size(tmp_files));

    memcpy(new_files,
           array::begin(tmp_files),
           sizeof(char *) * array::size(tmp_files));

    *files = new_files;
    *count = array::size(tmp_files);
}

void dir_list_free(char **files,
                   uint32_t count,
                   struct cel_alloc *allocator) {
    for (uint32_t i = 0; i < count; ++i) {
        CEL_FREE(allocator, files[i]);
    }

    CEL_FREE(allocator, files);
}


int dir_make(const char *path) {
    struct stat st;
    const int mode = 0775;

    if (stat(path, &st) != 0) {
        if (mkdir(path, mode) != 0 && errno != EEXIST) {
            return 0;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return 0;
    }

    return 1;
}

int dir_make_path(const char *path) {
    char *pp;
    char *sp;
    int status = 1;
    char *copypath = strdup(path);

    pp = copypath;
    while (status == 1 && (sp = strchr(pp, '/')) != 0) {
        if (sp != pp) {
            *sp = '\0';
            status = dir_make(copypath);
            *sp = '/';
        }

        pp = sp + 1;
    }

    if (status == 1) {
        status = dir_make(path);
    }

    free(copypath);
    return status;
}

const char *path_filename(const char *path) {
    char *ch = strrchr(path, DIR_DELIM_CH);
    return ch != NULL ? ch + 1 : path;
}

void path_basename(const char *path,
                   char *out) {
    const char *filename = path_filename(path);
    const char *ch = strrchr(filename, '.');

    if (ch == NULL) {
        memcpy(out, filename, strlen(filename));
        return;
    }

    const size_t basename_len = (ch - filename) / sizeof(char);
    memcpy(out, filename, basename_len);
}

void path_dir(char *out,
              const char *path) {
    char *ch = strrchr(path, DIR_DELIM_CH);

    if (ch != NULL) {
        size_t len = ch - path;
        memcpy(out, path, len);
        out[len] = '\0';
    }
}

const char *path_extension(const char *path) {
    const char *filename = path_filename(path);
    const char *ch = strrchr(filename, '.');

    if (ch == NULL) {
        return NULL;
    }

    return ch + 1;
}

char *path_join(struct cel_alloc *allocator,
                uint32_t count,
                ...) {

    Buffer buffer(allocator);

    va_list arguments;
    va_start (arguments, count);

    buffer << va_arg (arguments, const char*);

    for (uint32_t i = 1; i < count; ++i) {
        buffer << DIR_DELIM_STR;
        buffer << va_arg (arguments, const char*);
    }

    va_end (arguments);

    c_str(buffer);
    char *data = buffer._data;
    buffer._data = NULL;

    return data;
}

void copy_file(struct cel_alloc *allocator,
               const char *from,
               const char *to) {
    ct_vio *source_vio = ct_vio_a0.from_file(from, VIO_OPEN_READ);

    char *data = CEL_ALLOCATE(allocator, char,
                              source_vio->size(source_vio->inst));

    size_t size = (size_t) source_vio->size(source_vio->inst);
    source_vio->read(source_vio->inst, data, sizeof(char), size);
    source_vio->close(source_vio->inst);

    ct_vio *build_vio = ct_vio_a0.from_file(to, VIO_OPEN_WRITE);

    build_vio->write(build_vio->inst, data, sizeof(char), size);
    build_vio->close(build_vio->inst);

    CEL_FREE(allocator, data);
}

static ct_path_a0 path_api = {
        .list = dir_list,
        .list_free = dir_list_free,
        .make_path = dir_make_path,
        .filename = path_filename,
        .basename = path_basename,
        .dir = path_dir,
        .extension = path_extension,
        .join = path_join,
        .file_mtime = file_mtime,
        .copy_file = copy_file
};

CETECH_MODULE_DEF(
        path,
        {
            CETECH_GET_API(api, ct_log_a0);
            CETECH_GET_API(api, ct_vio_a0);
        },
        {
            api->register_api("ct_path_a0", &path_api);
        },
        {
            CEL_UNUSED(api);
        }
)