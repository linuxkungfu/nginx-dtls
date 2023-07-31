#ifndef _NGX_EVENT_UDP_H_INCLUDED_
#define _NGX_EVENT_UDP_H_INCLUDED_
#include <ngx_core.h>
#if !(NGX_WIN32)
struct ngx_udp_connection_s {
    ngx_rbtree_node_t   node;
    ngx_connection_t   *connection;
    ngx_buf_t          *buffer;
};
#endif
#endif