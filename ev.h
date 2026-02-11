#ifndef EV_H
#define EV_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int epfd;
    bool quit;
} ev_loop;

enum ev_type
{
    EV_IO,
    EV_SIGNAL,
};

typedef struct ev ev_data;
typedef void (*ev_callback) (ev_data *ev, uint32_t mask);

typedef struct ev
{
    enum ev_type type;
    ev_loop *loop;
    int fd;
    ev_callback cb;
    void *data;
} ev_data;

int ev_init (ev_loop *loop);
int ev_run (ev_loop *loop);
void ev_quit (ev_loop *loop);

ev_data *ev_register_io (ev_loop *loop, int fd, uint32_t events, ev_callback callback, void *data);
ev_data *ev_register_signal (ev_loop *loop, uint32_t signo, ev_callback callback, void *data);
int ev_remove (ev_data *ev);

#endif
