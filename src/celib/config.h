#ifndef CE_CONFIG_H
#define CE_CONFIG_H

#include <stdint.h>
#include <celib/module.inl>

struct ce_alloc;

struct ce_config_a0 {
    //! Parse commandline arguments.
    int (*parse_args)(int argc,
                      const char **argv);

    //! Load config from yaml file.
    int (*load_from_yaml_file)(const char *yaml,
                               struct ce_alloc *alloc);

    uint64_t (*obj)();

    //! Dump all variables to log
    void (*log_all)();
};

CE_MODULE(ce_config_a0);

#endif //CE_CONFIG_H
