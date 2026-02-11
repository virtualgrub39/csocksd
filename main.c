#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "ev.h"
#include "log.h"

prog_cfg
prog_cfg_default (void)
{
    return (prog_cfg){
        .log_file = (CFG_DEFAULT_LOGFILE),
        .port = (CFG_DEFAULT_PORT),
        .backlog = (CFG_DEFAULT_BACKLOG),
        .config_file_path = (CFG_DEFAULT_CONFIG_PATH),
    };
}

bool
progargs_parse (int argc, char *const *argv, prog_cfg *cfg)
{
    printf ("TODO: progargs_parse\n");
    return true;
}

bool
config_parse (prog_cfg *cfg)
{
    printf ("TODO: config_parse\n");
    return true;
}

typedef struct
{
    int fd;
    ev_loop loop;
    prog_cfg cfg;
} server_ctx;

static void
on_term_signal (ev_data *ev, uint32_t mask)
{
    log_info ("received signal %02X, exiting...\n", mask);
    ev->loop->quit = true;
}

static void
on_client_data (ev_data *ev, uint32_t events)
{
    int fd = (int)ev->data;

    char buffer[512];
    int result = recv (fd, buffer, sizeof buffer, 0);
    if (result == 0)
    {
        log_info ("client disconnected");
        close (fd);
        return;
    }
    if (result < 0)
    {
        if (errno == EWOULDBLOCK || errno == EINTR) return;
        log_errno ("failed to receive data from client");
        close (fd);
        return;
    }

    log_info ("received %d bytes of data from client", result);

    ev_remove (ev);
    close (fd);
    return;
}

static int
make_nonblocking (int fd)
{
    int flags = fcntl (fd, F_GETFL, 0);
    if (flags < 0) return flags;
    flags |= SOCK_NONBLOCK;
    return fcntl (fd, F_SETFL, flags);
}

static void
on_new_connection (ev_data *ev, uint32_t events)
{
    server_ctx *ctx = ev->data;

    if (events & EPOLLERR)
    {
        log_errno ("error in event loop");
        return;
    }

    int clientfd = accept (ctx->fd, NULL, NULL);
    if (clientfd < 0)
    {
        log_errno ("failed to accept client");
        return;
    }

    if (make_nonblocking (clientfd) < 0)
    {
        log_errno ("failed to modify client socket");
        return;
    }

    if (!ev_register_io (&ctx->loop, clientfd, EPOLLIN, on_client_data, (void *)clientfd))
    {
        close (clientfd);
        log_error ("failed to register client into event loop");
    }
}

bool
server_socket_init (const prog_cfg *cfg, int *sockfd)
{
    int fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
        log_errno ("failed to initialize socket");
        return false;
    }

    struct sockaddr_in bindaddr = { 0 };
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    bindaddr.sin_port = htons (cfg->port);

    int yes = 1;
    setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    // setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes);

    if (bind (fd, (struct sockaddr *)&bindaddr, sizeof bindaddr) < 0)
    {
        log_errno ("bind failed");
        close (fd);
        return false;
    }

    if (make_nonblocking (fd) < 0)
    {
        log_errno ("couldn't modify socket");
        close (fd);
        return false;
    }

    if (listen (fd, cfg->backlog) < 0)
    {
        log_errno ("couldn't listen on socket");
        close (fd);
        return false;
    }

    *sockfd = fd;
    return true;
}

bool
server_init (server_ctx *sctx, int argc, char *const *argv)
{
    assert (sctx != NULL);

    sctx->cfg = prog_cfg_default ();

    if (!progargs_parse (argc, argv, &sctx->cfg)) return false;
    if (!config_parse (&sctx->cfg)) return false;

    logger_init (&sctx->cfg);

    if (!server_socket_init (&sctx->cfg, &sctx->fd)) return false;

    if (ev_init (&sctx->loop) < 0)
    {
        close (sctx->fd);
        return false;
    }

    if (!ev_register_io (&sctx->loop, sctx->fd, EPOLLIN, on_new_connection, sctx))
    {
        log_errno ("failed to register socket callback");
        goto fail;
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
    close (sctx->fd);
    ev_quit (&sctx->loop);
    return false;
}

int
server_run (server_ctx *ctx)
{
    return ev_run (&ctx->loop);
}

void
server_cleanup (server_ctx *ctx)
{
    if (ctx->fd >= 0) close (ctx->fd);
    ev_quit (&ctx->loop);
}

int
main (int argc, char *argv[])
{
    server_ctx sctx = { 0 };
    if (!server_init (&sctx, argc, argv)) return 1;

    int retcode = server_run (&sctx);

    server_cleanup (&sctx);
    return retcode;
}
