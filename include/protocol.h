#ifndef PROTOCOL_H
#define PROTOCOL_H

#define SOCKS_AUTH_NONE 0x00
#define SOCKS_AUTH_GSSAPI 0x01
#define SOCKS_AUTH_PASSWD 0x02

#define SOCKS_CONNECT 0x01
#define SOCKS_BIND 0x02
#define SOCKS_UDP_ASSOCIATE 0x03

#define SOCKS_REP_SUCCESS 0x00
#define SOCKS_REP_GENERAL_FAILURE 0x01

#define SOCKS_ATYPE_IPV4 0x01
#define SOCKS_ATYPE_DOMAINNAME 0x03
#define SOCKS_ATYPE_IPV6 0x04

#include "session.h"

void socks_handshake (session_ctx *ctx);

#endif
