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

#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include <errno.h>

#define STRLEN 15

int main(int argc, char *argv[])
{
    apr_pool_t *context;
    apr_socket_t *sock;
    apr_size_t length;
    apr_status_t stat;
    char datasend[STRLEN] = "Send data test";
    char datarecv[STRLEN];
    char msgbuf[80];
    char *local_ipaddr, *remote_ipaddr;
    char *dest = "127.0.0.1";
    apr_port_t local_port, remote_port;
    apr_interval_time_t timeout = apr_time_from_sec(2);
    apr_sockaddr_t *local_sa, *remote_sa;

    setbuf(stdout, NULL);
    if (argc > 1) {
        dest = argv[1];
    }

    if (argc > 2) {
        timeout = atoi(argv[2]);
    }

    fprintf(stdout, "Initializing.........");
    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    atexit(apr_terminate);

    fprintf(stdout, "Creating context.......");
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout,"\tClient:  Making socket address...............");
    if ((stat = apr_sockaddr_info_get(&remote_sa, dest, APR_UNSPEC, 8021, 0, context)) 
        != APR_SUCCESS) {
        fprintf(stdout, "Failed!\n");
        fprintf(stdout, "Address resolution failed for %s: %s\n", 
                dest, apr_strerror(stat, msgbuf, sizeof(msgbuf)));
        exit(-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout, "\tClient:  Creating new socket.......");
    if (apr_socket_create(&sock, remote_sa->family, SOCK_STREAM,
                          context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't create socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Setting socket timeout.......");
    stat = apr_socket_timeout_set(sock, timeout);
    if (stat) {
        fprintf(stderr, "Problem setting timeout: %d\n", stat);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Connecting to socket.......");

    stat = apr_socket_connect(sock, remote_sa);

    if (stat != APR_SUCCESS) {
        apr_socket_close(sock);
        fprintf(stderr, "Could not connect: %s (%d)\n", 
		apr_strerror(stat, msgbuf, sizeof(msgbuf)), stat);
        fflush(stderr);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    apr_socket_addr_get(&remote_sa, APR_REMOTE, sock);
    apr_sockaddr_ip_get(&remote_ipaddr, remote_sa);
    apr_sockaddr_port_get(&remote_port, remote_sa);
    apr_socket_addr_get(&local_sa, APR_LOCAL, sock);
    apr_sockaddr_ip_get(&local_ipaddr, local_sa);
    apr_sockaddr_port_get(&local_port, local_sa);
    fprintf(stdout, "\tClient socket: %s:%u -> %s:%u\n", local_ipaddr, local_port, remote_ipaddr, remote_port);

    fprintf(stdout, "\tClient:  Trying to send data over socket.......");
    length = STRLEN;
    if ((stat = apr_socket_send(sock, datasend, &length) != APR_SUCCESS)) {
        apr_socket_close(sock);
        fprintf(stderr, "Problem sending data: %s (%d)\n",
		apr_strerror(stat, msgbuf, sizeof(msgbuf)), stat);
        exit(-1);
    }
    fprintf(stdout, "OK\n");
   
    length = STRLEN; 
    fprintf(stdout, "\tClient:  Trying to receive data over socket.......");

    if ((stat = apr_socket_recv(sock, datarecv, &length)) != APR_SUCCESS) {
        apr_socket_close(sock);
        fprintf(stderr, "Problem receiving data: %s (%d)\n", 
		apr_strerror(stat, msgbuf, sizeof(msgbuf)), stat);
        exit(-1);
    }
    if (strcmp(datarecv, "Recv data test")) {
        apr_socket_close(sock);
        fprintf(stderr, "I did not receive the correct data %s\n", datarecv);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Shutting down socket.......");
    if (apr_socket_shutdown(sock, APR_SHUTDOWN_WRITE) != APR_SUCCESS) {
        apr_socket_close(sock);
        fprintf(stderr, "Could not shutdown socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Closing down socket.......");
    if (apr_socket_close(sock) != APR_SUCCESS) {
        fprintf(stderr, "Could not shutdown socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    return 1;
}
