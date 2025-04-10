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

#include <stdio.h>
#include <stdlib.h>
#include "apr_file_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "test_apr.h"

static apr_pool_t *pool;
static char *testdata;
static int cleanup_called = 0;

static apr_status_t string_cleanup(void *data)
{
    cleanup_called = 1;
    return APR_SUCCESS;
}

static void set_userdata(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_pool_userdata_set(testdata, "TEST", string_cleanup, pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}

static void get_userdata(CuTest *tc)
{
    apr_status_t rv;
    char *retdata;

    rv = apr_pool_userdata_get((void **)&retdata, "TEST", pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertStrEquals(tc, retdata, testdata);
}

static void get_nonexistkey(CuTest *tc)
{
    apr_status_t rv;
    char *retdata;

    rv = apr_pool_userdata_get((void **)&retdata, "DOESNTEXIST", pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrEquals(tc, retdata, NULL);
}

static void post_pool_clear(CuTest *tc)
{
    apr_status_t rv;
    char *retdata;

    rv = apr_pool_userdata_get((void **)&retdata, "DOESNTEXIST", pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrEquals(tc, retdata, NULL);
}

CuSuite *testud(void)
{
    CuSuite *suite = CuSuiteNew("User Data");

    apr_pool_create(&pool, p);
    testdata = apr_pstrdup(pool, "This is a test\n");

    SUITE_ADD_TEST(suite, set_userdata);
    SUITE_ADD_TEST(suite, get_userdata);
    SUITE_ADD_TEST(suite, get_nonexistkey);

    apr_pool_clear(pool);

    SUITE_ADD_TEST(suite, post_pool_clear);

    return suite;
}

