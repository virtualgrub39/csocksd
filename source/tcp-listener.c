#include "tcp-listener.h"
#include "ev.h"
#include "log.h"

#include <fcntl.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/socket.h>

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
    tcp_listener *listener = ev->data;

    if (events & EPOLLERR)
    {
        log_errno ("error in event loop");
        return;
    }

    int clientfd = accept (listener->fd, NULL, NULL);
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

    bool result = listener->on_connection (listener, clientfd, listener->user_data);

    if (!result)
    {
        close (clientfd);
        return;
    }
}

tcp_listener *
tcp_listener_init (ev_loop *loop, uint16_t port, int backlog, tcp_callback on_connection, void *user_data)
{
    if (!loop)
    {
        errno = EINVAL;
        return false;
    }

    int fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
        log_errno ("failed to initialize socket");
        return false;
    }

    struct sockaddr_in bindaddr = { 0 };
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    bindaddr.sin_port = htons (port);

    int yes = 1;
    setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    if (bind (fd, (struct sockaddr *)&bindaddr, sizeof bindaddr) < 0)
    {
        log_errno ("failed to bind socket to port %hu", port);
        close (fd);
        return false;
    }

    if (make_nonblocking (fd) < 0)
    {
        log_errno ("failed to modify socket");
        close (fd);
        return false;
    }

    if (listen (fd, backlog) < 0)
    {
        log_errno ("failed to listen on socket");
        close (fd);
        return false;
    }

    tcp_listener *listener = malloc (sizeof *listener);

    ev_data *ev = ev_register_io (loop, fd, EPOLLIN, on_new_connection, listener);

    if (!ev)
    {
        log_errno ("failed to register TCP listener in event loop");
        free (listener);
        close (fd);
        return false;
    }

    listener->ev_handle = ev;
    listener->fd = fd;
    listener->on_connection = on_connection;
    listener->user_data = user_data;

    return listener;
}

void
tcp_listener_close (tcp_listener *listener)
{
    if (!listener) return;
    if (listener->fd >= 0) close (listener->fd);
    if (listener->ev_handle) ev_remove (listener->ev_handle);
    listener->fd = 0;
    listener->ev_handle = NULL;
}
