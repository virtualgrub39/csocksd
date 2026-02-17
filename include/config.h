#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define CFG_DEFAULT_LOGFILE stdout
#define CFG_DEFAULT_PORT 6969
#define CFG_DEFAULT_BACKLOG 1024
#define CFG_DEFAULT_CONFIG_PATH NULL

// 0 - no auth
// 1 - gssapi
// 2 - user/pass
// all combinations possible, ex: "02", "21", "120", etc.
// prefference by index.
#define CFG_DEFAULT_AUTH "0"

typedef struct
{
    FILE *log_file;
    uint16_t port;
    int backlog;
    const char *config_file_path;
    const char *auth;
} prog_cfg;

prog_cfg prog_cfg_default (void);
bool progargs_parse (int argc, char *const *argv, prog_cfg *cfg);
bool confile_parse (prog_cfg *cfg);

#endif
