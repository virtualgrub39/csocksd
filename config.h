#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define CFG_DEFAULT_LOGFILE stdout
#define CFG_DEFAULT_PORT 6969
#define CFG_DEFAULT_BACKLOG 100
#define CFG_DEFAULT_CONFIG_PATH NULL

typedef struct
{
    FILE *log_file;
    uint16_t port;
    int backlog;
    const char *config_file_path;
} prog_cfg;

prog_cfg prog_cfg_default (void);
bool progargs_parse (int argc, char *const *argv, prog_cfg *cfg);
bool confile_parse (prog_cfg *cfg);

#endif
