#include "protocol.h"
#include "ev.h"

void
socks_handshake (session_ctx *ctx)
{
    if (!ctx) return;

    for (;;)
    {

        switch (ctx->state)
        {
        case STATE_INITIAL: {
            if (ringbuf_len (ctx->rxbuf) < 2) return;

            // [ VERSION:1 ][ NMETHODS:1 ]

            uint8_t header[2];
            assert (ringbuf_read (ctx->rxbuf, header, 2) == 2);

            ctx->version = header[0];
            ctx->nmethods = header[1];

            if (ctx->version != 0x05)
            {
                log_trace ("refused connection: invalid protocol version: 0x%02X", ctx->version);
                session_close (ctx);
                return;
            }

            ctx->state = STATE_RX_AUTH_METHODS;

            log_trace ("intial phase complete: version: 0x%02X, nmethods: 0x%02X", ctx->version, ctx->nmethods);
        }
        break;
        case STATE_RX_AUTH_METHODS: {
            if (ringbuf_len (ctx->rxbuf) < ctx->nmethods)
            {
                log_trace ("STATE_RX_AUTH_METHODS: to few methods: %hhu (%hhu required)", ringbuf_len (ctx->rxbuf),
                           ctx->nmethods);
                return;
            }

            uint8_t methods[ctx->nmethods];
            assert (ringbuf_read (ctx->rxbuf, methods, ctx->nmethods) == ctx->nmethods);

            uint8_t method = 0xff;
            uint8_t out_buffer[2];

            for (const char *m = ctx->server_handle->cfg.auth; *m != '\0'; ++m)
            {
                log_trace ("trying: %c...", *m);

                for (uint8_t i = 0; i < ctx->nmethods; ++i)
                {
                    if (methods[i] == (*m) - '0')
                    {
                        method = methods[i];
                        goto method_accepted;
                    }
                }
            }

        method_accepted: // or loop broken (so not accepted)

            out_buffer[0] = ctx->version;
            out_buffer[1] = method;

            ringbuf_write (ctx->txbuf, out_buffer, sizeof out_buffer);
            ev_set_rw (ctx->ev_handle);

            switch (method)
            {
            case SOCKS_AUTH_NONE: ctx->state = STATE_RX_REQUEST_HEADER; break;
            case SOCKS_AUTH_GSSAPI:
                log_error ("TODO: GSSAPI auth");
                session_close (ctx);
                return;
            case SOCKS_AUTH_PASSWD:
                log_error ("TODO: USER/PASS auth");
                session_close (ctx);
                return;
            case 0xFF:
                log_trace ("no method accepted - closing connection");
                ctx->terminate = true;
                return;
            default:
                log_error ("Unknown auth method accepted: 0x%02X", method);
                session_close (ctx);
                return;
            }
        }
        break;
        case STATE_RX_REQUEST_HEADER: {
            if (ringbuf_len (ctx->rxbuf) < 4) return; // not enough data

            char buf[4];
            assert (ringbuf_read (ctx->rxbuf, buf, sizeof buf) == sizeof buf);

            ctx->rep = SOCKS_REP_SUCCESS;

            if (buf[0] != ctx->version) ctx->rep = SOCKS_REP_GENERAL_FAILURE;

            ctx->cmd = buf[1];
            ctx->atyp = buf[3];

            switch (ctx->atyp)
            {
            case SOCKS_ATYPE_IPV4: ctx->state = STATE_RX_REQUEST_ADDR_V4; break;
            case SOCKS_ATYPE_IPV6: ctx->state = STATE_RX_REQUEST_ADDR_V6; break;
            case SOCKS_ATYPE_DOMAINNAME: ctx->state = STATE_RX_REQUEST_ADDR_DOMAINNAME_LEN; break;
            default:
                log_trace ("Invalid packet format: invalid address type: 0x%02X", ctx->atyp);
                session_close (ctx);
                return;
            }
        }
        break;
        case STATE_RX_REQUEST_ADDR_V4: {
            if (ringbuf_len (ctx->rxbuf) < 4) return;

            ctx->addr_len = 4;
            assert (ringbuf_read (ctx->rxbuf, ctx->addr, 4) == 4);

            ctx->state = STATE_RX_REQUEST_PORT;
        }
        break;
        case STATE_RX_REQUEST_ADDR_V6: {
            if (ringbuf_len (ctx->rxbuf) < 16) return;

            ctx->addr_len = 16;
            assert (ringbuf_read (ctx->rxbuf, ctx->addr, 16) == 16);

            ctx->state = STATE_RX_REQUEST_PORT;
        }
        break;
        case STATE_RX_REQUEST_ADDR_DOMAINNAME_LEN: {
            if (ringbuf_len (ctx->rxbuf) == 0) return;

            assert (ringbuf_read (ctx->rxbuf, &ctx->addr_len, 1) == 1);

            ctx->state = STATE_RX_REQUEST_ADDR_DOMAINNAME;
        }
        break;
        case STATE_RX_REQUEST_ADDR_DOMAINNAME: {
            if (ringbuf_len (ctx->rxbuf) < ctx->addr_len) return;

            assert (ringbuf_read (ctx->rxbuf, ctx->addr, ctx->addr_len) == ctx->addr_len);

            ctx->state = STATE_RX_REQUEST_PORT;
        }
        break;
        case STATE_RX_REQUEST_PORT: {
            if (ringbuf_len (ctx->rxbuf) < 2) return;

            assert (ringbuf_read (ctx->rxbuf, &ctx->port, 2) == 2);

            ctx->state = STATE_PROCESS_REQUEST;
        }
        break;
        case STATE_PROCESS_REQUEST: {
            // TODO !!!
            session_close (ctx);
            return;
        }
        }
    }
}
