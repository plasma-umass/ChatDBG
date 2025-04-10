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

#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "test_apr.h"

/* XXX I'm sure there has to be a better way to do this ... */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif

#define TESTSTR "This is a test"

static apr_proc_t newproc;

static void test_create_proc(CuTest *tc)
{
    const char *args[2];
    apr_procattr_t *attr;
    apr_file_t *testfile = NULL;
    apr_status_t rv;
    apr_size_t length;
    char *buf;

    rv = apr_procattr_create(&attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_io_set(attr, APR_FULL_BLOCK, APR_FULL_BLOCK, 
                             APR_NO_PIPE);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_dir_set(attr, "data");
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    args[0] = "proc_child" EXTENSION;
    args[1] = NULL;
    
    rv = apr_proc_create(&newproc, "../proc_child" EXTENSION, args, NULL, 
                         attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    testfile = newproc.in;

    length = strlen(TESTSTR);
    rv = apr_file_write(testfile, TESTSTR, &length);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, strlen(TESTSTR), length);

    testfile = newproc.out;
    length = 256;
    buf = apr_pcalloc(p, length);
    rv = apr_file_read(testfile, buf, &length);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, TESTSTR, buf);
}

static void test_proc_wait(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_proc_wait(&newproc, NULL, NULL, APR_WAIT);
    CuAssertIntEquals(tc, APR_CHILD_DONE, rv);
}

static void test_file_redir(CuTest *tc)
{
    apr_file_t *testout = NULL;
    apr_file_t *testerr = NULL;
    apr_off_t offset;
    apr_status_t rv;
    const char *args[2];
    apr_procattr_t *attr;
    apr_file_t *testfile = NULL;
    apr_size_t length;
    char *buf;

    testfile = NULL;
    rv = apr_file_open(&testfile, "data/stdin",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&testout, "data/stdout",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&testerr, "data/stderr",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    length = strlen(TESTSTR);
    apr_file_write(testfile, TESTSTR, &length);
    offset = 0;
    rv = apr_file_seek(testfile, APR_SET, &offset);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, offset);

    rv = apr_procattr_create(&attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_in_set(attr, testfile, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_out_set(attr, testout, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_err_set(attr, testerr, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_dir_set(attr, "data");
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    args[0] = "proc_child";
    args[1] = NULL;

    rv = apr_proc_create(&newproc, "../proc_child" EXTENSION, args, NULL, 
                         attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_proc_wait(&newproc, NULL, NULL, APR_WAIT);
    CuAssertIntEquals(tc, APR_CHILD_DONE, rv);

    offset = 0;
    rv = apr_file_seek(testout, APR_SET, &offset);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    length = 256;
    buf = apr_pcalloc(p, length);
    rv = apr_file_read(testout, buf, &length);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, TESTSTR, buf);


    apr_file_close(testfile);
    apr_file_close(testout);
    apr_file_close(testerr);

    rv = apr_file_remove("data/stdin", p);;
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_remove("data/stdout", p);;
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_remove("data/stderr", p);;
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

CuSuite *testproc(void)
{
    CuSuite *suite = CuSuiteNew("Process control");

    SUITE_ADD_TEST(suite, test_create_proc);
    SUITE_ADD_TEST(suite, test_proc_wait);
    SUITE_ADD_TEST(suite, test_file_redir);

    return suite;
}

