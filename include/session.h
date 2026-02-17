#ifndef CLIENT_H
#define CLIENT_H

#include "csocks.h"
#include "ev.h"

typedef struct
{
    csocks_ctx *server_handle;
    ev_data *ev_handle;

    int fd;
    ringbuf *rxbuf, *txbuf;
    bool terminate;

    enum
    {
        STATE_INITIAL,
        STATE_RX_AUTH_METHODS,
        STATE_RX_REQUEST_HEADER,
        STATE_RX_REQUEST_ADDR_V4,
        STATE_RX_REQUEST_ADDR_V6,
        STATE_RX_REQUEST_ADDR_DOMAINNAME_LEN,
        STATE_RX_REQUEST_ADDR_DOMAINNAME,
        STATE_RX_REQUEST_PORT,
        STATE_PROCESS_REQUEST,
    } state;

    uint8_t version;
    uint8_t nmethods;

    uint8_t auth_method;

    uint8_t cmd;
    uint8_t rep;
    uint8_t atyp;

    uint8_t addr_len;
    uint8_t addr[256];

    uint16_t port;
} session_ctx;

// called by tcp-listener after accepting connection. runs client_init
bool on_new_connection (tcp_listener *listener, int fd, void *server);

// called on EPOLLIN/EPOLLOUT on client_ctx->fd
void on_data_available (ev_data *ev, uint32_t events);

session_ctx *session_init (csocks_ctx *server, int fd);
void session_close (session_ctx *ctx);

#endif
