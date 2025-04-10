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
#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_file_io.h"
#include "apr_strings.h"

/* hmmm, what is a truly portable define for the max path
 * length on a platform?
 */
#define PATH_LEN 255
#define TEST_STRING "This is the MMAP data file."APR_EOL_STR

#if !APR_HAS_MMAP
static void not_implemented(CuTest *tc)
{
    CuNotImpl(tc, "User functions");
}

#else

static apr_mmap_t *themmap = NULL;
static apr_file_t *thefile = NULL;
static char *file1;
static apr_finfo_t finfo;
static int fsize;

static void create_filename(CuTest *tc)
{
    char *oldfileptr;

    apr_filepath_get(&file1, 0, p);
#ifndef NETWARE
#ifdef WIN32
    CuAssertTrue(tc, file1[1] == ':');
#else
    CuAssertTrue(tc, file1[0] == '/');
#endif
#endif
    CuAssertTrue(tc, file1[strlen(file1) - 1] != '/');

    oldfileptr = file1;
    file1 = apr_pstrcat(p, file1,"/data/mmap_datafile.txt" ,NULL);
    CuAssertTrue(tc, oldfileptr != file1);
}

static void test_file_close(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_close(thefile);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}
   
static void test_file_open(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_open(&thefile, file1, APR_READ, APR_UREAD | APR_GREAD, p);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrNotNull(tc, thefile);
}
   
static void test_get_filesize(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertIntEquals(tc, fsize, finfo.size);
}

static void test_mmap_create(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_mmap_create(&themmap, thefile, 0, finfo.size, APR_MMAP_READ, p);
    CuAssertPtrNotNull(tc, themmap);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}

static void test_mmap_contents(CuTest *tc)
{
    
    CuAssertPtrNotNull(tc, themmap);
    CuAssertPtrNotNull(tc, themmap->mm);
    CuAssertIntEquals(tc, fsize, themmap->size);

    /* Must use nEquals since the string is not guaranteed to be NULL terminated */
    CuAssertStrNEquals(tc, themmap->mm, TEST_STRING, fsize);
}

static void test_mmap_delete(CuTest *tc)
{
    apr_status_t rv;

    CuAssertPtrNotNull(tc, themmap);
    rv = apr_mmap_delete(themmap);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}

static void test_mmap_offset(CuTest *tc)
{
    apr_status_t rv;
    void *addr;

    CuAssertPtrNotNull(tc, themmap);
    rv = apr_mmap_offset(&addr, themmap, 5);

    /* Must use nEquals since the string is not guaranteed to be NULL terminated */
    CuAssertStrNEquals(tc, addr, TEST_STRING + 5, fsize-5);
}
#endif

CuSuite *testmmap(void)
{
    CuSuite *suite = CuSuiteNew("MMAP");

#if APR_HAS_MMAP    
    fsize = strlen(TEST_STRING);

    SUITE_ADD_TEST(suite, create_filename);
    SUITE_ADD_TEST(suite, test_file_open);
    SUITE_ADD_TEST(suite, test_get_filesize);
    SUITE_ADD_TEST(suite, test_mmap_create);
    SUITE_ADD_TEST(suite, test_mmap_contents);
    SUITE_ADD_TEST(suite, test_mmap_offset);
    SUITE_ADD_TEST(suite, test_mmap_delete);
    SUITE_ADD_TEST(suite, test_file_close);
#else
    SUITE_ADD_TEST(suite, not_implemented);
#endif

    return suite;
}

