#ifndef CELIB_CMD_LINE_H
#define CELIB_CMD_LINE_H

struct args {
    int argc;
    const char **argv;
};

int cmd_find_argument(struct args args,
                          const char *longopt,
                          char shortopt);

const char *cmd_get_parameter(struct args args,
                                  const char *longopt,
                                  char shortopt,
                                  int param);

int cmd_has_argument(struct args args,
                         const char *longopt,
                         char shortopt);

#endif //CELIB_CMD_LINE_H
