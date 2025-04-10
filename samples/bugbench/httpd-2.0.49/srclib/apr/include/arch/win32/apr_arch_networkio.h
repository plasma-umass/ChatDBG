/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#include "apr_network_io.h"
#include "apr_general.h"

typedef struct sock_userdata_t sock_userdata_t;
struct sock_userdata_t {
    sock_userdata_t *next;
    const char *key;
    void *data;
};

struct apr_socket_t {
    apr_pool_t         *cntxt;
    SOCKET              socketdes;
    int                 type; /* SOCK_STREAM, SOCK_DGRAM */
    int                 protocol;
    apr_sockaddr_t     *local_addr;
    apr_sockaddr_t     *remote_addr;
    int                 timeout_ms; /* MUST MATCH if timeout > 0 */
    apr_interval_time_t timeout;
    apr_int32_t         disconnected;
    int                 local_port_unknown;
    int                 local_interface_unknown;
    int                 remote_addr_unknown;
    apr_int32_t         netmask;
    apr_int32_t         inherit;
    sock_userdata_t    *userdata;
};

#ifdef _WIN32_WCE
#ifndef WSABUF
typedef struct _WSABUF {
    u_long      len;     /* the length of the buffer */
    char FAR *  buf;     /* the pointer to the buffer */
} WSABUF, FAR * LPWSABUF;
#endif
#endif

apr_status_t status_from_res_error(int);

const char *apr_inet_ntop(int af, const void *src, char *dst, apr_size_t size);
int apr_inet_pton(int af, const char *src, void *dst);
void apr_sockaddr_vars_set(apr_sockaddr_t *, int, apr_port_t);

#define apr_is_option_set(mask, option)  ((mask & option) ==option)
#define apr_set_option(mask, option, on) \
    do {                                 \
        if (on)                          \
            *mask |= option;             \
        else                             \
            *mask &= ~option;            \
    } while (0)

#endif  /* ! NETWORK_IO_H */

