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
#include "ev.h"
#include "log.h"
#include "ringbuf.h"

static int
make_nonblocking (int fd)
{
    int flags = fcntl (fd, F_GETFL, 0);
    if (flags < 0) return flags;
    flags |= SOCK_NONBLOCK;
    return fcntl (fd, F_SETFL, flags);
}

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

struct server_ctx;
typedef struct server_ctx server_ctx;

typedef struct connection
{
    // references
    server_ctx *server_handle;
    ev_data *ev_handle;

    // owned
    int fd;
    ringbuf *rxbuf, *txbuf;

    // struct connection *next;
} connection;

struct server_ctx
{
    int fd;
    ev_loop loop;
    prog_cfg cfg;
    // connection *clients;
};

void
client_write_commit (connection *conn)
{
    ev_modify_io (conn->ev_handle, EPOLLIN | EPOLLOUT);
}

void
client_data_available (connection *conn)
{
    log_info ("received %d bytes of data from client", ringbuf_len (conn->rxbuf));

    char data[512];
    size_t n = ringbuf_read (conn->rxbuf, data, sizeof data);
    ringbuf_write (conn->txbuf, data, n);

    client_write_commit (conn);
}

void
connection_close (connection *c)
{
    close (c->fd);
    ringbuf_free (c->rxbuf);
    ringbuf_free (c->txbuf);
    ev_remove (c->ev_handle);
    free (c);
}

void
on_connection_data (ev_data *ev, uint32_t events)
{
    connection *conn = ev->data;

    if (events & EPOLLIN)
    {

        char buffer[512]; // FIXME: configurable
        int result = recv (conn->fd, buffer, sizeof buffer, 0);
        if (result == 0)
        {
            log_info ("client disconnected");
            goto close_connection;
        }
        if (result < 0)
        {
            if (errno == EWOULDBLOCK || errno == EINTR) return;
            log_errno ("failed to receive data from client");
            goto close_connection;
        }

        ringbuf_write (conn->rxbuf, buffer, result);

        client_data_available (conn);

        return;
    }

    else if (events & EPOLLOUT)
    {
        size_t len = ringbuf_len (conn->txbuf);

        if (len == 0)
        {
            ev_modify_io (ev, EPOLLIN);
            return;
        }

        struct iovec iov[2];
        int iovcnt = ringbuf_peek_iov (conn->txbuf, iov);
        if (iovcnt == 0)
        {
            ev_modify_io (ev, EPOLLIN);
            return;
        }

        ssize_t n = writev (conn->fd, iov, iovcnt);

        if (n > 0)
        {
            ringbuf_consume (conn->txbuf, (size_t)n);
            if (ringbuf_len (conn->txbuf) == 0) { ev_modify_io (ev, EPOLLIN); }
            return;
        }

        if (n == 0)
        {
            log_info ("peer closed");
            goto close_connection;
        }

        if (errno == EWOULDBLOCK || errno == EAGAIN) return;
        if (errno == EINTR) return;

        log_errno ("send/writev failed");
    }

close_connection:
    connection_close (conn);
}

connection *
connection_init (server_ctx *ctx, int fd)
{
    connection *c = calloc (1, sizeof *c);
    if (!c) return NULL;

    c->server_handle = ctx;
    c->fd = fd;

    ringbuf *rxbuf = ringbuf_new (1024); // FIXME: configurable
    ringbuf *txbuf = ringbuf_new (1024); // FIXME: configurable

    if (rxbuf == NULL || txbuf == NULL) goto err;

    c->rxbuf = rxbuf;
    c->txbuf = txbuf;

    c->ev_handle = ev_register_io (&ctx->loop, fd, EPOLLIN, on_connection_data, c);
    if (!c->ev_handle) goto err;

    return c;

err:
    ringbuf_free (rxbuf);
    ringbuf_free (txbuf);
    free (c);
    return NULL;
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

    connection *conn = connection_init (ctx, clientfd);
    if (!conn)
    {
        close (clientfd);
        log_error ("failed to initialize connection context");
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

static void
on_term_signal (ev_data *ev, uint32_t mask)
{
    log_info ("received signal %02X, exiting...\n", mask);
    ev->loop->quit = true;
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
