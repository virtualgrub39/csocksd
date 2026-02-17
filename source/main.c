#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "config.h"
#include "csocks.h"
#include "log.h"

prog_cfg
prog_cfg_default (void)
{
    return (prog_cfg){
        .log_file = (CFG_DEFAULT_LOGFILE),
        .port = (CFG_DEFAULT_PORT),
        .backlog = (CFG_DEFAULT_BACKLOG),
        .config_file_path = (CFG_DEFAULT_CONFIG_PATH),
        .auth = CFG_DEFAULT_AUTH,
    };
}

bool
progargs_parse (int argc, char *const *argv, prog_cfg *cfg)
{
    printf ("TODO: progargs_parse\n");
    return true;
}

bool
confile_parse (prog_cfg *cfg)
{
    printf ("TODO: config_parse\n");
    return true;
}

int
main (int argc, char *argv[])
{
    csocks_ctx sctx = { 0 };
    if (!csocks_init (&sctx, argc, argv)) return 1;

    log_info ("Listenting on port %hu", sctx.cfg.port);

    int retcode = csocks_run (&sctx);

    csocks_cleanup (&sctx);
    return retcode;
}
