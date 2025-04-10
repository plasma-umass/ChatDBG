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

#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"

#if APR_HAVE_IPV6
#define US "::1"
#define FAMILY APR_INET6
#else
#define US "127.0.0.1"
#define FAMILY APR_INET
#endif

#define STRLEN 21

static void tcp_socket(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
}

static void udp_socket(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
}

static void tcp6_socket(CuTest *tc)
{
#if APR_HAVE_IPV6
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET6, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
#else
    CuNotImpl(tc, "IPv6");
#endif
}

static void udp6_socket(CuTest *tc)
{
#if APR_HAVE_IPV6
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET6, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
#else
    CuNotImpl(tc, "IPv6");
#endif
}

static void sendto_receivefrom(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock = NULL;
    apr_socket_t *sock2 = NULL;
    char sendbuf[STRLEN] = "APR_INET, SOCK_DGRAM";
    char recvbuf[80];
    char *ip_addr;
    apr_port_t fromport;
    apr_sockaddr_t *from;
    apr_sockaddr_t *to;
    apr_size_t len = 30;

    rv = apr_socket_create(&sock, FAMILY, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_socket_create(&sock2, FAMILY, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_sockaddr_info_get(&to, US, APR_UNSPEC, 7772, 0, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_sockaddr_info_get(&from, US, APR_UNSPEC, 7771, 0, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_bind(sock, to);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_socket_bind(sock2, from);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    len = STRLEN;
    rv = apr_socket_sendto(sock2, to, 0, sendbuf, &len);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, STRLEN, len);

    len = 80;
    rv = apr_socket_recvfrom(from, sock, 0, recvbuf, &len);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, STRLEN, len);
    CuAssertStrEquals(tc, "APR_INET, SOCK_DGRAM", recvbuf);

    apr_sockaddr_ip_get(&ip_addr, from);
    apr_sockaddr_port_get(&fromport, from);
    CuAssertStrEquals(tc, US, ip_addr);
    CuAssertIntEquals(tc, 7771, fromport);

    apr_socket_close(sock);
    apr_socket_close(sock2);
}

static void socket_userdata(CuTest *tc)
{
    apr_socket_t *sock1, *sock2;
    apr_status_t rv;
    char *data;
    const char *key = "GENERICKEY";

    rv = apr_socket_create(&sock1, AF_INET, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_socket_create(&sock2, AF_INET, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_data_set(sock1, "SOCK1", key, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_socket_data_set(sock2, "SOCK2", key, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_data_get((void **)&data, key, sock1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, "SOCK1", data);
    rv = apr_socket_data_get((void **)&data, key, sock2);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, "SOCK2", data);
}

CuSuite *testsockets(void)
{
    CuSuite *suite = CuSuiteNew("Socket Creation");

    SUITE_ADD_TEST(suite, tcp_socket);
    SUITE_ADD_TEST(suite, udp_socket);

    SUITE_ADD_TEST(suite, tcp6_socket);
    SUITE_ADD_TEST(suite, udp6_socket);

    SUITE_ADD_TEST(suite, sendto_receivefrom);

    SUITE_ADD_TEST(suite, socket_userdata);
    
    return suite;
}

