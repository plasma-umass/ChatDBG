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


#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_file_io.h"
#include "test_apr.h"

#define TEST "Testing\n"
#define TEST2 "Testing again\n"
#define FILEPATH "data/"

static void test_file_dup(CuTest *tc)
{
    apr_file_t *file1 = NULL;
    apr_file_t *file3 = NULL;
    apr_status_t rv;
    apr_finfo_t finfo;

    /* First, create a new file, empty... */
    rv = apr_file_open(&file1, FILEPATH "testdup.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, file1);

    rv = apr_file_dup(&file3, file1, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, file3);

    rv = apr_file_close(file1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* cleanup after ourselves */
    rv = apr_file_close(file3);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_stat(&finfo, FILEPATH "testdup.file", APR_FINFO_NORM, p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}  

static void test_file_readwrite(CuTest *tc)
{
    apr_file_t *file1 = NULL;
    apr_file_t *file3 = NULL;
    apr_status_t rv;
    apr_finfo_t finfo;
    apr_size_t txtlen = sizeof(TEST);
    char buff[50];
    apr_off_t fpos;

    /* First, create a new file, empty... */
    rv = apr_file_open(&file1, FILEPATH "testdup.readwrite.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, file1);

    rv = apr_file_dup(&file3, file1, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, file3);

    rv = apr_file_write(file3, TEST, &txtlen);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, sizeof(TEST), txtlen);

    fpos = 0;
    rv = apr_file_seek(file1, APR_SET, &fpos);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, fpos);

    txtlen = 50;
    rv = apr_file_read(file1, buff, &txtlen);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, TEST, buff);

    /* cleanup after ourselves */
    rv = apr_file_close(file1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_close(file3);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_stat(&finfo, FILEPATH "testdup.readwrite.file", APR_FINFO_NORM, p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}  

static void test_dup2(CuTest *tc)
{
    apr_file_t *testfile = NULL;
    apr_file_t *errfile = NULL;
    apr_file_t *saveerr = NULL;
    apr_status_t rv;

    rv = apr_file_open(&testfile, FILEPATH "testdup2.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, testfile);

    rv = apr_file_open_stderr(&errfile, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* Set aside the real errfile */
    rv = apr_file_dup(&saveerr, errfile, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, saveerr);

    rv = apr_file_dup2(errfile, testfile, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, errfile);

    apr_file_close(testfile);

    rv = apr_file_dup2(errfile, saveerr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, errfile);
}

static void test_dup2_readwrite(CuTest *tc)
{
    apr_file_t *errfile = NULL;
    apr_file_t *testfile = NULL;
    apr_file_t *saveerr = NULL;
    apr_status_t rv;
    apr_size_t txtlen = sizeof(TEST);
    char buff[50];
    apr_off_t fpos;

    rv = apr_file_open(&testfile, FILEPATH "testdup2.readwrite.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, testfile);

    rv = apr_file_open_stderr(&errfile, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* Set aside the real errfile */
    rv = apr_file_dup(&saveerr, errfile, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, saveerr);

    rv = apr_file_dup2(errfile, testfile, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, errfile);

    txtlen = sizeof(TEST2);
    rv = apr_file_write(errfile, TEST2, &txtlen);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, sizeof(TEST2), txtlen);

    fpos = 0;
    rv = apr_file_seek(testfile, APR_SET, &fpos);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, fpos);

    txtlen = 50;
    rv = apr_file_read(testfile, buff, &txtlen);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, TEST2, buff);
      
    apr_file_close(testfile);

    rv = apr_file_dup2(errfile, saveerr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, errfile);
}

CuSuite *testdup(void)
{
    CuSuite *suite = CuSuiteNew("File duplication");

    SUITE_ADD_TEST(suite, test_file_dup);
    SUITE_ADD_TEST(suite, test_file_readwrite);
    SUITE_ADD_TEST(suite, test_dup2);
    SUITE_ADD_TEST(suite, test_dup2_readwrite);

    return suite;
}

