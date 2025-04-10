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

#include "test_apr.h"
#include "apr_strings.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_network_io.h"
#include "apr_poll.h"

#define SMALL_NUM_SOCKETS 3
/* We can't use 64 here, because some platforms *ahem* Solaris *ahem* have
 * a default limit of 64 open file descriptors per process.  If we use
 * 64, the test will fail even though the code is correct.
 */
#define LARGE_NUM_SOCKETS 50

static apr_socket_t *s[LARGE_NUM_SOCKETS];
static apr_sockaddr_t *sa[LARGE_NUM_SOCKETS];
static apr_pollfd_t *pollarray;
static apr_pollfd_t *pollarray_large;
static apr_pollset_t *pollset;

static void make_socket(apr_socket_t **sock, apr_sockaddr_t **sa, 
                        apr_port_t port, apr_pool_t *p, CuTest *tc)
{
    apr_status_t rv;

    rv = apr_sockaddr_info_get(sa, "127.0.0.1", APR_UNSPEC, port, 0, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_create(sock, (*sa)->family, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv =apr_socket_bind((*sock), (*sa));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void check_sockets(const apr_pollfd_t *pollarray, 
                          apr_socket_t **sockarray, int which, int pollin, 
                          CuTest *tc)
{
    apr_status_t rv;
    apr_int16_t event;
    char *str;

    rv = apr_poll_revents_get(&event, sockarray[which], 
                              (apr_pollfd_t *)pollarray);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    if (pollin) {
        str = apr_psprintf(p, "Socket %d not signalled when it should be",
                           which);
        CuAssert(tc, str, event & APR_POLLIN);
    } else {
        str = apr_psprintf(p, "Socket %d signalled when it should not be",
                           which);
        CuAssert(tc, str, !(event & APR_POLLIN));
    }
}

static void send_msg(apr_socket_t **sockarray, apr_sockaddr_t **sas, int which,
                     CuTest *tc)
{
    apr_size_t len = 5;
    apr_status_t rv;

    CuAssertPtrNotNull(tc, sockarray[which]);

    rv = apr_socket_sendto(sockarray[which], sas[which], 0, "hello", &len);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, strlen("hello"), len);
}

static void recv_msg(apr_socket_t **sockarray, int which, apr_pool_t *p, 
                     CuTest *tc)
{
    apr_size_t buflen = 5;
    char *buffer = apr_pcalloc(p, sizeof(char) * (buflen + 1));
    apr_sockaddr_t *recsa;
    apr_status_t rv;

    CuAssertPtrNotNull(tc, sockarray[which]);

    apr_sockaddr_info_get(&recsa, "127.0.0.1", APR_UNSPEC, 7770, 0, p);

    rv = apr_socket_recvfrom(recsa, sockarray[which], 0, buffer, &buflen);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, strlen("hello"), buflen);
    CuAssertStrEquals(tc, "hello", buffer);
}

    
static void create_all_sockets(CuTest *tc)
{
    int i;

    for (i = 0; i < LARGE_NUM_SOCKETS; i++){
        make_socket(&s[i], &sa[i], 7777 + i, p, tc);
    }
}
       
static void setup_small_poll(CuTest *tc)
{
    apr_status_t rv;
    int i;

    rv = apr_poll_setup(&pollarray, SMALL_NUM_SOCKETS, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    for (i = 0; i < SMALL_NUM_SOCKETS;i++){
        CuAssertIntEquals(tc, 0, pollarray[i].reqevents);
        CuAssertIntEquals(tc, 0, pollarray[i].rtnevents);

        rv = apr_poll_socket_add(pollarray, s[i], APR_POLLIN);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
        CuAssertPtrEquals(tc, s[i], pollarray[i].desc.s);
    }
}

static void setup_large_poll(CuTest *tc)
{
    apr_status_t rv;
    int i;

    rv = apr_poll_setup(&pollarray_large, LARGE_NUM_SOCKETS, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    for (i = 0; i < LARGE_NUM_SOCKETS;i++){
        CuAssertIntEquals(tc, 0, pollarray_large[i].reqevents);
        CuAssertIntEquals(tc, 0, pollarray_large[i].rtnevents);

        rv = apr_poll_socket_add(pollarray_large, s[i], APR_POLLIN);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
        CuAssertPtrEquals(tc, s[i], pollarray_large[i].desc.s);
    }
}

static void nomessage(CuTest *tc)
{
    apr_status_t rv;
    int srv = SMALL_NUM_SOCKETS;

    rv = apr_poll(pollarray, SMALL_NUM_SOCKETS, &srv, 2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    check_sockets(pollarray, s, 0, 0, tc);
    check_sockets(pollarray, s, 1, 0, tc);
    check_sockets(pollarray, s, 2, 0, tc);
}

static void send_2(CuTest *tc)
{
    apr_status_t rv;
    int srv = SMALL_NUM_SOCKETS;

    send_msg(s, sa, 2, tc);

    rv = apr_poll(pollarray, SMALL_NUM_SOCKETS, &srv, 2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    check_sockets(pollarray, s, 0, 0, tc);
    check_sockets(pollarray, s, 1, 0, tc);
    check_sockets(pollarray, s, 2, 1, tc);
}

static void recv_2_send_1(CuTest *tc)
{
    apr_status_t rv;
    int srv = SMALL_NUM_SOCKETS;

    recv_msg(s, 2, p, tc);
    send_msg(s, sa, 1, tc);

    rv = apr_poll(pollarray, SMALL_NUM_SOCKETS, &srv, 2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    check_sockets(pollarray, s, 0, 0, tc);
    check_sockets(pollarray, s, 1, 1, tc);
    check_sockets(pollarray, s, 2, 0, tc);
}

static void send_2_signaled_1(CuTest *tc)
{
    apr_status_t rv;
    int srv = SMALL_NUM_SOCKETS;

    send_msg(s, sa, 2, tc);

    rv = apr_poll(pollarray, SMALL_NUM_SOCKETS, &srv, 2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    check_sockets(pollarray, s, 0, 0, tc);
    check_sockets(pollarray, s, 1, 1, tc);
    check_sockets(pollarray, s, 2, 1, tc);
}

static void recv_1_send_0(CuTest *tc)
{
    apr_status_t rv;
    int srv = SMALL_NUM_SOCKETS;

    recv_msg(s, 1, p, tc);
    send_msg(s, sa, 0, tc);

    rv = apr_poll(pollarray, SMALL_NUM_SOCKETS, &srv, 2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    check_sockets(pollarray, s, 0, 1, tc);
    check_sockets(pollarray, s, 1, 0, tc);
    check_sockets(pollarray, s, 2, 1, tc);
}

static void clear_all_signalled(CuTest *tc)
{
    apr_status_t rv;
    int srv = SMALL_NUM_SOCKETS;

    recv_msg(s, 0, p, tc);
    recv_msg(s, 2, p, tc);

    rv = apr_poll(pollarray, SMALL_NUM_SOCKETS, &srv, 2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    check_sockets(pollarray, s, 0, 0, tc);
    check_sockets(pollarray, s, 1, 0, tc);
    check_sockets(pollarray, s, 2, 0, tc);
}

static void send_large_pollarray(CuTest *tc)
{
    apr_status_t rv;
    int lrv = LARGE_NUM_SOCKETS;
    int i;

    send_msg(s, sa, LARGE_NUM_SOCKETS - 1, tc);

    rv = apr_poll(pollarray_large, LARGE_NUM_SOCKETS, &lrv, 
                  2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    for (i = 0; i < LARGE_NUM_SOCKETS; i++) {
        if (i == (LARGE_NUM_SOCKETS - 1)) {
            check_sockets(pollarray_large, s, i, 1, tc);
        }
        else {
            check_sockets(pollarray_large, s, i, 0, tc);
        }
    }
}

static void recv_large_pollarray(CuTest *tc)
{
    apr_status_t rv;
    int lrv = LARGE_NUM_SOCKETS;
    int i;

    recv_msg(s, LARGE_NUM_SOCKETS - 1, p, tc);

    rv = apr_poll(pollarray_large, LARGE_NUM_SOCKETS, &lrv, 
                  2 * APR_USEC_PER_SEC);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));

    for (i = 0; i < LARGE_NUM_SOCKETS; i++) {
        check_sockets(pollarray_large, s, i, 0, tc);
    }
}

static void setup_pollset(CuTest *tc)
{
    apr_status_t rv;
    rv = apr_pollset_create(&pollset, LARGE_NUM_SOCKETS, p, 0);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void add_sockets_pollset(CuTest *tc)
{
    apr_status_t rv;
    int i;

    for (i = 0; i < LARGE_NUM_SOCKETS;i++){
        apr_pollfd_t socket_pollfd;

        CuAssertPtrNotNull(tc, s[i]);

        socket_pollfd.desc_type = APR_POLL_SOCKET;
        socket_pollfd.reqevents = APR_POLLIN;
        socket_pollfd.desc.s = s[i];
        socket_pollfd.client_data = s[i];
        rv = apr_pollset_add(pollset, &socket_pollfd);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
    }
}

static void nomessage_pollset(CuTest *tc)
{
    apr_status_t rv;
    int lrv;
    const apr_pollfd_t *descs = NULL;

    rv = apr_pollset_poll(pollset, 0, &lrv, &descs);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    CuAssertIntEquals(tc, 0, lrv);
    CuAssertPtrEquals(tc, NULL, descs);
}

static void send0_pollset(CuTest *tc)
{
    apr_status_t rv;
    const apr_pollfd_t *descs = NULL;
    int num;
    
    send_msg(s, sa, 0, tc);
    rv = apr_pollset_poll(pollset, 0, &num, &descs);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, num);
    CuAssertPtrNotNull(tc, descs);

    CuAssertPtrEquals(tc, s[0], descs[0].desc.s);
    CuAssertPtrEquals(tc, s[0],  descs[0].client_data);
}

static void recv0_pollset(CuTest *tc)
{
    apr_status_t rv;
    int lrv;
    const apr_pollfd_t *descs = NULL;

    recv_msg(s, 0, p, tc);
    rv = apr_pollset_poll(pollset, 0, &lrv, &descs);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    CuAssertIntEquals(tc, 0, lrv);
    CuAssertPtrEquals(tc, NULL, descs);
}

static void send_middle_pollset(CuTest *tc)
{
    apr_status_t rv;
    const apr_pollfd_t *descs = NULL;
    int num;
    
    send_msg(s, sa, 2, tc);
    send_msg(s, sa, 5, tc);
    rv = apr_pollset_poll(pollset, 0, &num, &descs);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 2, num);
    CuAssertPtrNotNull(tc, descs);

    CuAssert(tc, "Incorrect socket in result set",
            ((descs[0].desc.s == s[2]) && (descs[1].desc.s == s[5])) ||
            ((descs[0].desc.s == s[5]) && (descs[1].desc.s == s[2])));
}

static void clear_middle_pollset(CuTest *tc)
{
    apr_status_t rv;
    int lrv;
    const apr_pollfd_t *descs = NULL;

    recv_msg(s, 2, p, tc);
    recv_msg(s, 5, p, tc);

    rv = apr_pollset_poll(pollset, 0, &lrv, &descs);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    CuAssertIntEquals(tc, 0, lrv);
    CuAssertPtrEquals(tc, NULL, descs);
}

static void send_last_pollset(CuTest *tc)
{
    apr_status_t rv;
    const apr_pollfd_t *descs = NULL;
    int num;
    
    send_msg(s, sa, LARGE_NUM_SOCKETS - 1, tc);
    rv = apr_pollset_poll(pollset, 0, &num, &descs);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, num);
    CuAssertPtrNotNull(tc, descs);

    CuAssertPtrEquals(tc, s[LARGE_NUM_SOCKETS - 1], descs[0].desc.s);
    CuAssertPtrEquals(tc, s[LARGE_NUM_SOCKETS - 1],  descs[0].client_data);
}

static void clear_last_pollset(CuTest *tc)
{
    apr_status_t rv;
    int lrv;
    const apr_pollfd_t *descs = NULL;

    recv_msg(s, LARGE_NUM_SOCKETS - 1, p, tc);

    rv = apr_pollset_poll(pollset, 0, &lrv, &descs);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    CuAssertIntEquals(tc, 0, lrv);
    CuAssertPtrEquals(tc, NULL, descs);
}

static void close_all_sockets(CuTest *tc)
{
    apr_status_t rv;
    int i;

    for (i = 0; i < LARGE_NUM_SOCKETS; i++){
        rv = apr_socket_close(s[i]);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
    }
}

static void pollset_remove(CuTest *tc)
{
    apr_status_t rv;
    apr_pollset_t *pollset;
    const apr_pollfd_t *hot_files;
    apr_pollfd_t pfd;
    apr_int32_t num;

    rv = apr_pollset_create(&pollset, 5, p, 0);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    pfd.p = p;
    pfd.desc_type = APR_POLL_SOCKET;
    pfd.reqevents = APR_POLLOUT;

    pfd.desc.s = s[0];
    pfd.client_data = (void *)1;
    rv = apr_pollset_add(pollset, &pfd);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    pfd.desc.s = s[1];
    pfd.client_data = (void *)2;
    rv = apr_pollset_add(pollset, &pfd);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    pfd.desc.s = s[2];
    pfd.client_data = (void *)3;
    rv = apr_pollset_add(pollset, &pfd);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    pfd.desc.s = s[1];
    pfd.client_data = (void *)4;
    rv = apr_pollset_add(pollset, &pfd);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    pfd.desc.s = s[3];
    pfd.client_data = (void *)5;
    rv = apr_pollset_add(pollset, &pfd);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_pollset_poll(pollset, 1000, &num, &hot_files);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 5, num);

    /* now remove the pollset elements referring to desc s[1] */
    pfd.desc.s = s[1];
    pfd.client_data = (void *)999; /* not used on this call */
    rv = apr_pollset_remove(pollset, &pfd);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* this time only three should match */
    rv = apr_pollset_poll(pollset, 1000, &num, &hot_files);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 3, num);
    CuAssertPtrEquals(tc, (void *)1, hot_files[0].client_data);
    CuAssertPtrEquals(tc, s[0], hot_files[0].desc.s);
    CuAssertPtrEquals(tc, (void *)3, hot_files[1].client_data);
    CuAssertPtrEquals(tc, s[2], hot_files[1].desc.s);
    CuAssertPtrEquals(tc, (void *)5, hot_files[2].client_data);
    CuAssertPtrEquals(tc, s[3], hot_files[2].desc.s);
    
    /* now remove the pollset elements referring to desc s[2] */
    pfd.desc.s = s[2];
    pfd.client_data = (void *)999; /* not used on this call */
    rv = apr_pollset_remove(pollset, &pfd);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* this time only two should match */
    rv = apr_pollset_poll(pollset, 1000, &num, &hot_files);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 2, num);
    CuAssertPtrEquals(tc, (void *)1, hot_files[0].client_data);
    CuAssertPtrEquals(tc, s[0], hot_files[0].desc.s);
    CuAssertPtrEquals(tc, (void *)5, hot_files[1].client_data);
    CuAssertPtrEquals(tc, s[3], hot_files[1].desc.s);
}

CuSuite *testpoll(void)
{
    CuSuite *suite = CuSuiteNew("Poll");

    SUITE_ADD_TEST(suite, create_all_sockets);
    SUITE_ADD_TEST(suite, setup_small_poll);
    SUITE_ADD_TEST(suite, setup_large_poll);
    SUITE_ADD_TEST(suite, nomessage);
    SUITE_ADD_TEST(suite, send_2);
    SUITE_ADD_TEST(suite, recv_2_send_1);
    SUITE_ADD_TEST(suite, send_2_signaled_1);
    SUITE_ADD_TEST(suite, recv_1_send_0);
    SUITE_ADD_TEST(suite, clear_all_signalled);
    SUITE_ADD_TEST(suite, send_large_pollarray);
    SUITE_ADD_TEST(suite, recv_large_pollarray);

    SUITE_ADD_TEST(suite, setup_pollset);
    SUITE_ADD_TEST(suite, add_sockets_pollset);
    SUITE_ADD_TEST(suite, nomessage_pollset);
    SUITE_ADD_TEST(suite, send0_pollset);
    SUITE_ADD_TEST(suite, recv0_pollset);
    SUITE_ADD_TEST(suite, send_middle_pollset);
    SUITE_ADD_TEST(suite, clear_middle_pollset);
    SUITE_ADD_TEST(suite, send_last_pollset);
    SUITE_ADD_TEST(suite, clear_last_pollset);

    SUITE_ADD_TEST(suite, pollset_remove);
    
    SUITE_ADD_TEST(suite, close_all_sockets);

    return suite;
}

