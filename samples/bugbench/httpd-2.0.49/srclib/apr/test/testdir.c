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
#include <string.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"

static void test_mkdir(CuTest *tc)
{
    apr_status_t rv;
    apr_finfo_t finfo;

    rv = apr_dir_make("data/testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_stat(&finfo, "data/testdir", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);
}

static void test_mkdir_recurs(CuTest *tc)
{
    apr_status_t rv;
    apr_finfo_t finfo;

    rv = apr_dir_make_recursive("data/one/two/three", 
                                APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_stat(&finfo, "data/one", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);

    rv = apr_stat(&finfo, "data/one/two", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);

    rv = apr_stat(&finfo, "data/one/two/three", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);
}

static void test_remove(CuTest *tc)
{
    apr_status_t rv;
    apr_finfo_t finfo;

    rv = apr_dir_remove("data/testdir", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_stat(&finfo, "data/testdir", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}

static void test_removeall_fail(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_remove("data/one", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOTEMPTY(rv));
}

static void test_removeall(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_remove("data/one/two/three", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_remove("data/one/two", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_remove("data/one", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_remove_notthere(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_remove("data/notthere", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}

static void test_mkdir_twice(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_make("data/testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_make("data/testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EEXIST(rv));

    rv = apr_dir_remove("data/testdir", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_opendir(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;

    rv = apr_dir_open(&dir, "data", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    apr_dir_close(dir);
}

static void test_opendir_notthere(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;

    rv = apr_dir_open(&dir, "notthere", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}

static void test_closedir(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;

    rv = apr_dir_open(&dir, "data", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_close(dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_rewind(CuTest *tc)
{
    apr_dir_t *dir;
    apr_finfo_t first, second;

    apr_assert_success(tc, "apr_dir_open failed", apr_dir_open(&dir, "data", p));

    apr_assert_success(tc, "apr_dir_read failed",
                       apr_dir_read(&first, APR_FINFO_DIRENT, dir));

    apr_assert_success(tc, "apr_dir_rewind failed", apr_dir_rewind(dir));

    apr_assert_success(tc, "second apr_dir_read failed",
                       apr_dir_read(&second, APR_FINFO_DIRENT, dir));

    apr_assert_success(tc, "apr_dir_close failed", apr_dir_close(dir));

    CuAssertStrEquals(tc, first.name, second.name);
}

/* Test for a (fixed) bug in apr_dir_read().  This bug only happened
   in threadless cases. */
static void test_uncleared_errno(CuTest *tc)
{
    apr_file_t *thefile = NULL;
    apr_finfo_t finfo;
    apr_int32_t finfo_flags = APR_FINFO_TYPE | APR_FINFO_NAME;
    apr_dir_t *this_dir;
    apr_status_t rv; 

    rv = apr_dir_make("dir1", APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_make("dir2", APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&thefile, "dir1/file1",
                       APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_close(thefile);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* Try to remove dir1.  This should fail because it's not empty.
       However, on a platform with threads disabled (such as FreeBSD),
       `errno' will be set as a result. */
    rv = apr_dir_remove("dir1", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOTEMPTY(rv));
    
    /* Read `.' and `..' out of dir2. */
    rv = apr_dir_open(&this_dir, "dir2", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_read(&finfo, finfo_flags, this_dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_read(&finfo, finfo_flags, this_dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* Now, when we attempt to do a third read of empty dir2, and the
       underlying system readdir() returns NULL, the old value of
       errno shouldn't cause a false alarm.  We should get an ENOENT
       back from apr_dir_read, and *not* the old errno. */
    rv = apr_dir_read(&finfo, finfo_flags, this_dir);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));

    rv = apr_dir_close(this_dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
		 
    /* Cleanup */
    rv = apr_file_remove("dir1/file1", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_remove("dir1", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_remove("dir2", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

}

CuSuite *testdir(void)
{
    CuSuite *suite = CuSuiteNew("Directory");

    SUITE_ADD_TEST(suite, test_mkdir);
    SUITE_ADD_TEST(suite, test_mkdir_recurs);
    SUITE_ADD_TEST(suite, test_remove);
    SUITE_ADD_TEST(suite, test_removeall_fail);
    SUITE_ADD_TEST(suite, test_removeall);
    SUITE_ADD_TEST(suite, test_remove_notthere);
    SUITE_ADD_TEST(suite, test_mkdir_twice);

    SUITE_ADD_TEST(suite, test_rewind);

    SUITE_ADD_TEST(suite, test_opendir);
    SUITE_ADD_TEST(suite, test_opendir_notthere);
    SUITE_ADD_TEST(suite, test_closedir);
    SUITE_ADD_TEST(suite, test_uncleared_errno);

    return suite;
}

