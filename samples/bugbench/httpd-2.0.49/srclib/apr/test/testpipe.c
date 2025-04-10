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

#include "test_apr.h"
#include "apr_file_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_thread_proc.h"
#include "apr_strings.h"

static apr_file_t *readp = NULL;
static apr_file_t *writep = NULL;

static void create_pipe(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);
}   

static void close_pipe(CuTest *tc)
{
    apr_status_t rv;
    apr_size_t nbytes = 256;
    char buf[256];

    rv = apr_file_close(readp);
    rv = apr_file_close(writep);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_read(readp, buf, &nbytes);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EBADF(rv));
}   

static void set_timeout(CuTest *tc)
{
    apr_status_t rv;
    apr_file_t *readp = NULL;
    apr_file_t *writep = NULL;
    apr_interval_time_t timeout;

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);

    rv = apr_file_pipe_timeout_get(readp, &timeout);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, -1, timeout);

    rv = apr_file_pipe_timeout_set(readp, apr_time_from_sec(1));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_pipe_timeout_get(readp, &timeout);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, apr_time_from_sec(1), timeout);
}

static void read_write(CuTest *tc)
{
    apr_status_t rv;
    char *buf;
    apr_size_t nbytes;
    
    nbytes = strlen("this is a test");
    buf = (char *)apr_palloc(p, nbytes + 1);

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);

    rv = apr_file_pipe_timeout_set(readp, apr_time_from_sec(1));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_read(readp, buf, &nbytes);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    CuAssertIntEquals(tc, 0, nbytes);
}

static void read_write_notimeout(CuTest *tc)
{
    apr_status_t rv;
    char *buf = "this is a test";
    char *input;
    apr_size_t nbytes;
    
    nbytes = strlen("this is a test");

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);

    rv = apr_file_write(writep, buf, &nbytes);
    CuAssertIntEquals(tc, strlen("this is a test"), nbytes);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    nbytes = 256;
    input = apr_pcalloc(p, nbytes + 1);
    rv = apr_file_read(readp, input, &nbytes);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, strlen("this is a test"), nbytes);
    CuAssertStrEquals(tc, "this is a test", input);
}

/* XXX FIXME */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif

static void test_pipe_writefull(CuTest *tc)
{
    int iterations = 1000;
    int i;
    int bytes_per_iteration = 8000;
    char *buf = (char *)malloc(bytes_per_iteration);
    char responsebuf[128];
    apr_size_t nbytes;
    int bytes_processed;
    apr_proc_t proc = {0};
    apr_procattr_t *procattr;
    const char *args[2];
    apr_status_t rv;
    
    rv = apr_procattr_create(&procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK,
                             APR_CHILD_BLOCK);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_error_check_set(procattr, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    args[0] = "readchild" EXTENSION;
    args[1] = NULL;
    rv = apr_proc_create(&proc, "./readchild" EXTENSION, args, NULL, procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_pipe_timeout_set(proc.in, apr_time_from_sec(10));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_pipe_timeout_set(proc.out, apr_time_from_sec(10));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    i = iterations;
    do {
        rv = apr_file_write_full(proc.in, buf, bytes_per_iteration, NULL);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
    } while (--i);

    free(buf);

    rv = apr_file_close(proc.in);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    nbytes = sizeof(responsebuf);
    rv = apr_file_read(proc.out, responsebuf, &nbytes);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    bytes_processed = (int)apr_strtoi64(responsebuf, NULL, 10);
    CuAssertIntEquals(tc, iterations * bytes_per_iteration, bytes_processed);
}

CuSuite *testpipe(void)
{
    CuSuite *suite = CuSuiteNew("Pipes");

    SUITE_ADD_TEST(suite, create_pipe);
    SUITE_ADD_TEST(suite, close_pipe);
    SUITE_ADD_TEST(suite, set_timeout);
    SUITE_ADD_TEST(suite, read_write);
    SUITE_ADD_TEST(suite, read_write_notimeout);
    SUITE_ADD_TEST(suite, test_pipe_writefull);

    return suite;
}

