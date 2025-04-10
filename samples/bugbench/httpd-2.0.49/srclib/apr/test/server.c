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

#define APR_TEST_PREFIX "server: "

#include "aprtest.h"
#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_getopt.h"
#include "apr_poll.h"

#define STRLEN 15

int main(int argc, const char * const argv[])
{
    apr_pool_t *context;
    apr_status_t rv;
    apr_socket_t *sock;
    apr_socket_t *sock2;
    apr_size_t length;
    apr_int32_t pollres;
    apr_pollfd_t *sdset;
    char datasend[STRLEN];
    char datarecv[STRLEN] = "Recv data test";
    const char *bind_to_ipaddr = NULL;
    char *local_ipaddr, *remote_ipaddr;
    apr_port_t local_port, remote_port;
    apr_sockaddr_t *localsa = NULL, *remotesa;
    apr_status_t stat;
    int family = APR_UNSPEC;
    int protocol;
    apr_getopt_t *opt;
    const char *optarg;
    char optchar;

    APR_TEST_INITIALIZE(rv, context);

    APR_TEST_SUCCESS(rv, "Preparing getopt", 
                     apr_getopt_init(&opt, context, argc, argv))
    
    while ((stat = apr_getopt(opt, "i:", &optchar, &optarg)) == APR_SUCCESS) {
        switch(optchar) {
        case 'i':
            bind_to_ipaddr = optarg;
            break;
        }
    }
    if (stat != APR_EOF) {
        fprintf(stderr,
                "usage: %s [-i local-interface-address]\n",
                argv[0]);
        exit(-1);
    }

    if (bind_to_ipaddr) {
        /* First, parse/resolve ipaddr so we know what address family of
         * socket we need.  We'll use the returned sockaddr later when
         * we bind.
         */
        APR_TEST_SUCCESS(rv, "Preparing sockaddr", 
            apr_sockaddr_info_get(&localsa, bind_to_ipaddr, APR_UNSPEC, 8021, 0, context))
        family = localsa->family;
    }

    APR_TEST_SUCCESS(rv, "Creating new socket", 
        apr_socket_create_ex(&sock, family, SOCK_STREAM, APR_PROTO_TCP, context))

    APR_TEST_SUCCESS(rv, "Setting option APR_SO_NONBLOCK",
        apr_socket_opt_set(sock, APR_SO_NONBLOCK, 1))

    APR_TEST_SUCCESS(rv, "Setting option APR_SO_REUSEADDR",
        apr_socket_opt_set(sock, APR_SO_REUSEADDR, 1))

    if (!localsa) {
        apr_socket_addr_get(&localsa, APR_LOCAL, sock);
        apr_sockaddr_port_set(localsa, 8021);
    }

    APR_TEST_SUCCESS(rv, "Binding socket to port",
        apr_socket_bind(sock, localsa))
    
    APR_TEST_SUCCESS(rv, "Listening to socket",
        apr_socket_listen(sock, 5))
    
    APR_TEST_BEGIN(rv, "Setting up for polling",
        apr_poll_setup(&sdset, 1, context))
    APR_TEST_END(rv, 
        apr_poll_socket_add(sdset, sock, APR_POLLIN))
    
    pollres = 1; 
    APR_TEST_BEGIN(rv, "Polling for socket",
        apr_poll(sdset, 1, &pollres, -1))

    if (pollres == 0) {
        fprintf(stdout, "Failed\n");
        apr_socket_close(sock);
        fprintf(stderr, "Error: Unrecognized poll result, "
                "expected 1, received %d\n", pollres);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    APR_TEST_SUCCESS(rv, "Accepting a connection",
        apr_socket_accept(&sock2, sock, context))

    apr_socket_protocol_get(sock2, &protocol);
    if (protocol != APR_PROTO_TCP) {
        fprintf(stderr, "Error: protocol not conveyed from listening socket "
                "to connected socket!\n");
        exit(1);
    }
    apr_socket_addr_get(&remotesa, APR_REMOTE, sock2);
    apr_sockaddr_ip_get(&remote_ipaddr, remotesa);
    apr_sockaddr_port_get(&remote_port, remotesa);
    apr_socket_addr_get(&localsa, APR_LOCAL, sock2);
    apr_sockaddr_ip_get(&local_ipaddr, localsa);
    apr_sockaddr_port_get(&local_port, localsa);
    fprintf(stdout, "Server socket: %s:%u -> %s:%u\n", local_ipaddr, 
            local_port, remote_ipaddr, remote_port);

    APR_TEST_SUCCESS(rv, "Setting timeout on client socket",
        apr_socket_timeout_set(sock2, apr_time_from_sec(3)));

    length = STRLEN;
    APR_TEST_BEGIN(rv, "Receiving data from socket",
        apr_socket_recv(sock2, datasend, &length))

    if (strcmp(datasend, "Send data test")) {
        fprintf(stdout, "Failed\n");
        apr_socket_close(sock);
        apr_socket_close(sock2);
        fprintf(stderr, "Error: Unrecognized response;\n"
                "Expected: \"Send data test\"\n"
                "Received: \"%s\"\n", datarecv);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    length = STRLEN;
    APR_TEST_SUCCESS(rv, "Sending data over socket",
        apr_socket_send(sock2, datarecv, &length))
    
    APR_TEST_SUCCESS(rv, "Shutting down accepted socket",
        apr_socket_shutdown(sock2, APR_SHUTDOWN_READ))

    APR_TEST_SUCCESS(rv, "Closing duplicate socket",
        apr_socket_close(sock2))
    
    APR_TEST_SUCCESS(rv, "Closing original socket",
        apr_socket_close(sock))

    return 0;
}

