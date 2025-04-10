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

static apr_socket_t *sock = NULL;

static void create_socket(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_socket_create(&sock, APR_INET, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
}

static void set_keepalive(CuTest *tc)
{
    apr_status_t rv;
    apr_int32_t ck;

    rv = apr_socket_opt_set(sock, APR_SO_KEEPALIVE, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);
}

static void set_debug(CuTest *tc)
{
    apr_status_t rv1, rv2;
    apr_int32_t ck;
    
    /* On some platforms APR_SO_DEBUG can only be set as root; just test
     * for get/set consistency of this option. */
    rv1 = apr_socket_opt_set(sock, APR_SO_DEBUG, 1);
    rv2 = apr_socket_opt_get(sock, APR_SO_DEBUG, &ck);
    apr_assert_success(tc, "get SO_DEBUG option", rv2);
    if (APR_STATUS_IS_SUCCESS(rv1)) {
        CuAssertIntEquals(tc, 1, ck);
    } else {
        CuAssertIntEquals(tc, 0, ck);
    }
}

static void remove_keepalive(CuTest *tc)
{
    apr_status_t rv;
    apr_int32_t ck;

    rv = apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);

    rv = apr_socket_opt_set(sock, APR_SO_KEEPALIVE, 0);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, ck);
}

static void corkable(CuTest *tc)
{
#if !APR_HAVE_CORKABLE_TCP
    CuNotImpl(tc, "TCP isn't corkable");
#else
    apr_status_t rv;
    apr_int32_t ck;

    rv = apr_socket_opt_set(sock, APR_TCP_NODELAY, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);

    rv = apr_socket_opt_set(sock, APR_TCP_NOPUSH, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_TCP_NOPUSH, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);

    rv = apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, ck);

    rv = apr_socket_opt_set(sock, APR_TCP_NOPUSH, 0);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    rv = apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);
#endif
}

static void close_socket(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_socket_close(sock);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

CuSuite *testsockopt(void)
{
    CuSuite *suite = CuSuiteNew("Socket Options");

    SUITE_ADD_TEST(suite, create_socket);
    SUITE_ADD_TEST(suite, set_keepalive);
    SUITE_ADD_TEST(suite, set_debug);
    SUITE_ADD_TEST(suite, remove_keepalive);
    SUITE_ADD_TEST(suite, corkable);
    SUITE_ADD_TEST(suite, close_socket);

    return suite;
}

