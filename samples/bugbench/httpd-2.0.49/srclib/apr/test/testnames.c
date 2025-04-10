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
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_lib.h"

#if WIN32
#define ABS_ROOT "C:/"
#elif defined(NETWARE)
#define ABS_ROOT "SYS:/"
#else
#define ABS_ROOT "/"
#endif

static void merge_aboveroot(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;
    char errmsg[256];

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo", ABS_ROOT"bar", APR_FILEPATH_NOTABOVEROOT,
                            p);
    apr_strerror(rv, errmsg, sizeof(errmsg));
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EABOVEROOT(rv));
    CuAssertPtrEquals(tc, NULL, dstpath);
    CuAssertStrEquals(tc, "The given path was above the root path", errmsg);
}

static void merge_belowroot(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo", ABS_ROOT"foo/bar", 
                            APR_FILEPATH_NOTABOVEROOT, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/bar", dstpath);
}

static void merge_noflag(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo", ABS_ROOT"foo/bar", 0, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/bar", dstpath);
}

static void merge_dotdot(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../baz", 0, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/baz", dstpath);

    rv = apr_filepath_merge(&dstpath, "", "../test", 0, p);
    CuAssertIntEquals(tc, 0, APR_SUCCESS);
    CuAssertStrEquals(tc, "../test", dstpath);

    /* Very dangerous assumptions here about what the cwd is.  However, let's assume
     * that the testall is invoked from within apr/test/ so the following test should
     * return ../test unless a previously fixed bug remains or the developer changes
     * the case of the test directory:
     */
    rv = apr_filepath_merge(&dstpath, "", "../test", APR_FILEPATH_TRUENAME, p);
    CuAssertIntEquals(tc, 0, APR_SUCCESS);
    CuAssertStrEquals(tc, "../test", dstpath);
}

static void merge_secure(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../bar/baz", 0, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/bar/baz", dstpath);
}

static void merge_notrel(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../baz",
                            APR_FILEPATH_NOTRELATIVE, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/baz", dstpath);
}

static void merge_notrelfail(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;
    char errmsg[256];

    rv = apr_filepath_merge(&dstpath, "foo/bar", "../baz", 
                            APR_FILEPATH_NOTRELATIVE, p);
    apr_strerror(rv, errmsg, sizeof(errmsg));

    CuAssertPtrEquals(tc, NULL, dstpath);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ERELATIVE(rv));
    CuAssertStrEquals(tc, "The given path is relative", errmsg);
}

static void merge_notabsfail(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;
    char errmsg[256];

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../baz", 
                            APR_FILEPATH_NOTABSOLUTE, p);
    apr_strerror(rv, errmsg, sizeof(errmsg));

    CuAssertPtrEquals(tc, NULL, dstpath);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EABSOLUTE(rv));
    CuAssertStrEquals(tc, "The given path is absolute", errmsg);
}

static void merge_notabs(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, "foo/bar", "../baz", 
                            APR_FILEPATH_NOTABSOLUTE, p);

    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, "foo/baz", dstpath);
}

static void root_absolute(CuTest *tc)
{
    apr_status_t rv;
    const char *root = NULL;
    const char *path = ABS_ROOT"foo/bar";

    rv = apr_filepath_root(&root, &path, 0, p);

    CuAssertPtrNotNull(tc, root);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT, root);
}

static void root_relative(CuTest *tc)
{
    apr_status_t rv;
    const char *root = NULL;
    const char *path = "foo/bar";
    char errmsg[256];

    rv = apr_filepath_root(&root, &path, 0, p);
    apr_strerror(rv, errmsg, sizeof(errmsg));

    CuAssertPtrEquals(tc, NULL, root);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ERELATIVE(rv));
    CuAssertStrEquals(tc, "The given path is relative", errmsg);
}


#if 0
    root_result(rootpath);
    root_result(addpath);
}
#endif

CuSuite *testnames(void)
{
    CuSuite *suite = CuSuiteNew("Path names");

    SUITE_ADD_TEST(suite, merge_aboveroot);
    SUITE_ADD_TEST(suite, merge_belowroot);
    SUITE_ADD_TEST(suite, merge_noflag);
    SUITE_ADD_TEST(suite, merge_dotdot);
    SUITE_ADD_TEST(suite, merge_secure);
    SUITE_ADD_TEST(suite, merge_notrel);
    SUITE_ADD_TEST(suite, merge_notrelfail);
    SUITE_ADD_TEST(suite, merge_notabs);
    SUITE_ADD_TEST(suite, merge_notabsfail);

    SUITE_ADD_TEST(suite, root_absolute);
    SUITE_ADD_TEST(suite, root_relative);

    return suite;
}

