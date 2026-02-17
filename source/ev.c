#include "ev.h"
#include "log.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>

int
ev_remove (ev_data *ev)
{
    int e = epoll_ctl (ev->loop->epfd, EPOLL_CTL_DEL, ev->fd, NULL);
    free (ev);
    return e;
}

int
ev_init (ev_loop *loop)
{
    int epfd = epoll_create1 (0);
    if (epfd < 0) return epfd;

    loop->quit = false;
    loop->epfd = epfd;

    return 0;
}

void
ev_quit (ev_loop *loop)
{
    loop->quit = true;
    if (loop->epfd > 0) close (loop->epfd);
}

int
ev_run (ev_loop *loop)
{
    const size_t event_count = 16;
    struct epoll_event events[event_count];

    while (!loop->quit)
    {
        int evn = epoll_wait (loop->epfd, events, event_count, -1);
        if (evn < 0)
        {
            if (errno == EINTR) continue;
            log_errno ("epoll wait failed");
            return 1;
        }

        for (int i = 0; i < evn; ++i)
        {
            ev_data *ev = (ev_data *)(events[i].data.ptr);
            ev->cb (ev, events[i].events);
        }
    }

    return 0;
}

ev_data *
ev_register_signal (ev_loop *loop, uint32_t signo, ev_callback callback, void *data)
{
    assert (loop != NULL);

    sigset_t smask;
    sigemptyset (&smask);
    sigaddset (&smask, signo);

    if (sigprocmask (SIG_BLOCK, &smask, NULL) < 0)
    {
        log_errno ("Failed to block default signal handler");
        return NULL;
    }

    int fd = signalfd (-1, &smask, SFD_NONBLOCK);
    if (fd < 0)
    {
        log_errno ("Failed to create signal fd");
        return NULL;
    }

    ev_data *ev = calloc (1, sizeof *ev);
    if (!ev)
    {
        close (fd);
        return NULL;
    }

    ev->type = EV_SIGNAL;
    ev->fd = fd;
    ev->loop = loop;
    ev->cb = callback;
    ev->data = data;

    struct epoll_event epev = {
        .events = EPOLLIN,
        .data.ptr = ev,
    };

    if (epoll_ctl (loop->epfd, EPOLL_CTL_ADD, fd, &epev) < 0)
    {
        log_errno ("Failed to register signal fd into event loop");
        close (fd);
        free (ev);
        return NULL;
    }

    return ev;
}

ev_data *
ev_register_io (ev_loop *loop, int fd, uint32_t events, ev_callback callback, void *data)
{
    assert (loop != NULL);

    ev_data *ev = calloc (1, sizeof *ev);
    if (!ev) return NULL;

    ev->type = EV_IO;
    ev->loop = loop;
    ev->cb = callback;
    ev->data = data;
    ev->fd = fd;

    struct epoll_event epev = {
        .events = events,
        .data.ptr = ev,
    };

    if (epoll_ctl (loop->epfd, EPOLL_CTL_ADD, fd, &epev) < 0)
    {
        log_errno ("Failed to register fd into event loop");
        free (ev);
        return NULL;
    }

    return ev;
}

int
ev_modify_io (ev_data *ev, uint32_t events)
{
    struct epoll_event epev = {
        .events = events,
        .data.ptr = ev,
    };

    return epoll_ctl (ev->loop->epfd, EPOLL_CTL_MOD, ev->fd, &epev);
}
