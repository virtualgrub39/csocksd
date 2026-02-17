#include "csocks.h"
#include "config.h"
#include "session.h"

#include <signal.h>

static void
on_term_signal (ev_data *ev, uint32_t mask)
{
    log_info ("received signal %02X, exiting...\n", mask);
    ev->loop->quit = true;
}

bool
csocks_init (csocks_ctx *sctx, int argc, char *const *argv)
{
    assert (sctx != NULL);

    sctx->cfg = prog_cfg_default ();

    if (!progargs_parse (argc, argv, &sctx->cfg)) return false;
    if (!confile_parse (&sctx->cfg)) return false;

    logger_init (&sctx->cfg);

    if (ev_init (&sctx->loop) < 0) return false;

    if (!tcp_listener_init (&sctx->loop, sctx->cfg.port, sctx->cfg.backlog, on_new_connection, &sctx->listener))
    {
        ev_quit (&sctx->loop);
        return false;
    }

    if (!ev_register_signal (&sctx->loop, SIGINT, on_term_signal, sctx))
    {
        log_errno ("failed to register SIGINT callback");
        goto fail;
    }

    if (!ev_register_signal (&sctx->loop, SIGTERM, on_term_signal, sctx))
    {
        log_errno ("failed to register SIGTERM callback");
        goto fail;
    }

    return true;

fail:
    ev_quit (&sctx->loop);
    return false;
}

int
csocks_run (csocks_ctx *ctx)
{
    return ev_run (&ctx->loop);
}

void
csocks_cleanup (csocks_ctx *ctx)
{
    tcp_listener_close (&ctx->listener);
    ev_quit (&ctx->loop);
}
