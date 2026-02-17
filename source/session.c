#include "session.h"
#include "protocol.h"

#include <sys/epoll.h>

session_ctx *
session_init (csocks_ctx *server, int fd)
{
    session_ctx *ctx = calloc (1, sizeof *ctx);
    if (!ctx) return NULL;

    ringbuf *rxbuf = ringbuf_new (1024);
    ringbuf *txbuf = ringbuf_new (1024);

    if (!rxbuf || !txbuf) goto err;

    ev_data *ev = ev_register_io (&server->loop, fd, EPOLLIN, on_data_available, ctx);
    if (!ev) goto err;

    ctx->server_handle = server;
    ctx->ev_handle = ev;

    ctx->state = STATE_INITIAL;
    ctx->fd = fd;
    ctx->rxbuf = rxbuf;
    ctx->txbuf = txbuf;
    ctx->terminate = false;

    return ctx;
err:
    ringbuf_free (rxbuf);
    ringbuf_free (txbuf);
    return NULL;
}

void
session_close (session_ctx *ctx)
{
    if (!ctx) return;

    ev_remove (ctx->ev_handle);

    ringbuf_free (ctx->rxbuf);
    ringbuf_free (ctx->txbuf);

    close (ctx->fd);

    free (ctx);
    return;
}

bool
on_new_connection (tcp_listener *listener, int fd, void *data)
{
    log_trace ("new connection");
    csocks_ctx *server = data;
    session_ctx *client = session_init (server, fd);
    if (!client) return false;
    return true;
}

void
on_data_available (ev_data *ev, uint32_t events)
{
    session_ctx *conn = ev->data;

    if (events & EPOLLIN)
    {
        size_t space = ringbuf_space (conn->rxbuf);
        if (space == 0) return;

        char buffer[space];
        int result = recv (conn->fd, buffer, space, 0);
        if (result == 0) goto close_connection;
        if (result < 0)
        {
            if (errno == EWOULDBLOCK || errno == EINTR) return;
            log_errno ("failed to receive data from client");
            goto close_connection;
        }

        log_trace ("received %d bytes", result);

        ringbuf_write (conn->rxbuf, buffer, result);
        socks_handshake (conn);
    }

    if (events & EPOLLOUT)
    {
        size_t len = ringbuf_len (conn->txbuf);
        struct iovec iov[2];
        int iovcnt = ringbuf_peek_iov (conn->txbuf, iov);

        if (len == 0 || iovcnt == 0)
        {
            if (conn->terminate) goto close_connection;
            ev_set_r (ev);
            return;
        }

        ssize_t n = writev (conn->fd, iov, iovcnt);

        if (n > 0)
        {
            log_trace ("sent %d bytes", n);

            ringbuf_consume (conn->txbuf, (size_t)n);
            if (ringbuf_len (conn->rxbuf) > 0) socks_handshake (conn);
            if (ringbuf_len (conn->txbuf) == 0)
            {
                if (conn->terminate) goto close_connection;
                ev_set_r (ev);
            }
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
        goto close_connection;
    }

    return;

close_connection:
    session_close (conn);
}
