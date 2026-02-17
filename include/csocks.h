#ifndef CSOCKS_H
#define CSOCKS_H

#include "config.h"
#include "ev.h"
#include "tcp-listener.h"

typedef struct csocks_ctx
{
    tcp_listener listener;
    ev_loop loop;
    prog_cfg cfg;
} csocks_ctx;

bool csocks_init (csocks_ctx *sctx, int argc, char *const *argv);
int csocks_run (csocks_ctx *ctx);
void csocks_cleanup (csocks_ctx *ctx);

#endif
