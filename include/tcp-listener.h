#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "ev.h"
#include "log.h"
#include "ringbuf.h"

#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

struct tcp_listener;
typedef struct tcp_listener tcp_listener;

typedef bool (*tcp_callback) (struct tcp_listener *listener, int fd, void *data);

struct tcp_listener
{
    // references
    ev_data *ev_handle;
    void *user_data;

    // owned
    int fd;
    tcp_callback on_connection;
};

tcp_listener *tcp_listener_init (ev_loop *loop, uint16_t port, int backlog, tcp_callback on_connection,
                                 void *user_data);

void tcp_listener_close (tcp_listener *listener);

#endif
