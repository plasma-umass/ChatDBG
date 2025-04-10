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

#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_strings.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_poll.h"
#include "apr_lib.h"
#include "test_apr.h"

#define FILENAME "data/file_datafile.txt"
#define NEWFILENAME "data/new_datafile.txt"
#define NEWFILEDATA "This is new text in a new file."

static const struct view_fileinfo
{
    apr_int32_t bits;
    char *description;
} vfi[] = {
    {APR_FINFO_MTIME,  "MTIME"},
    {APR_FINFO_CTIME,  "CTIME"},
    {APR_FINFO_ATIME,  "ATIME"},
    {APR_FINFO_SIZE,   "SIZE"},
    {APR_FINFO_DEV,    "DEV"},
    {APR_FINFO_INODE,  "INODE"},
    {APR_FINFO_NLINK,  "NLINK"},
    {APR_FINFO_TYPE,   "TYPE"},
    {APR_FINFO_USER,   "USER"}, 
    {APR_FINFO_GROUP,  "GROUP"}, 
    {APR_FINFO_UPROT,  "UPROT"}, 
    {APR_FINFO_GPROT,  "GPROT"},
    {APR_FINFO_WPROT,  "WPROT"},
    {0,                NULL}
}; 

static void finfo_equal(CuTest *tc, apr_finfo_t f1, apr_finfo_t f2)
{
    /* Minimum supported flags across all platforms (APR_FINFO_MIN) */
    CuAssert(tc, "apr_stat and apr_getfileinfo must return APR_FINFO_TYPE",
             (f1.valid & f2.valid & APR_FINFO_TYPE));
    CuAssert(tc, "apr_stat and apr_getfileinfo differ in filetype",
             f1.filetype == f2.filetype);
    CuAssert(tc, "apr_stat and apr_getfileinfo must return APR_FINFO_SIZE",
             (f1.valid & f2.valid & APR_FINFO_SIZE));
    CuAssert(tc, "apr_stat and apr_getfileinfo differ in size",
             f1.size == f2.size);
    CuAssert(tc, "apr_stat and apr_getfileinfo must return APR_FINFO_ATIME",
             (f1.valid & f2.valid & APR_FINFO_ATIME));
    CuAssert(tc, "apr_stat and apr_getfileinfo differ in atime",
             f1.atime == f2.atime);
    CuAssert(tc, "apr_stat and apr_getfileinfo must return APR_FINFO_MTIME",
             (f1.valid & f2.valid & APR_FINFO_MTIME));
    CuAssert(tc, "apr_stat and apr_getfileinfo differ in mtime",
             f1.mtime == f2.mtime);
    CuAssert(tc, "apr_stat and apr_getfileinfo must return APR_FINFO_CTIME",
             (f1.valid & f2.valid & APR_FINFO_CTIME));
    CuAssert(tc, "apr_stat and apr_getfileinfo differ in ctime",
             f1.ctime == f2.ctime);

    if (f1.valid & f2.valid & APR_FINFO_NAME)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in name",
                 !strcmp(f1.name, f2.name));
    if (f1.fname && f2.fname)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in fname",
                 !strcmp(f1.fname, f2.fname));

    /* Additional supported flags not supported on all platforms */
    if (f1.valid & f2.valid & APR_FINFO_USER)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in user",
                 !apr_uid_compare(f1.user, f2.user));
    if (f1.valid & f2.valid & APR_FINFO_GROUP)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in group",
                 !apr_gid_compare(f1.group, f2.group));
    if (f1.valid & f2.valid & APR_FINFO_INODE)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in inode",
                 f1.inode == f2.inode);
    if (f1.valid & f2.valid & APR_FINFO_DEV)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in device",
                 f1.device == f2.device);
    if (f1.valid & f2.valid & APR_FINFO_NLINK)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in nlink",
                 f1.nlink == f2.nlink);
    if (f1.valid & f2.valid & APR_FINFO_CSIZE)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in csize",
                 f1.csize == f2.csize);
    if (f1.valid & f2.valid & APR_FINFO_PROT)
        CuAssert(tc, "apr_stat and apr_getfileinfo differ in protection",
                 f1.protection == f2.protection);
}

static void test_info_get(CuTest *tc)
{
    apr_file_t *thefile;
    apr_finfo_t finfo;
    apr_status_t rv;

    rv = apr_file_open(&thefile, FILENAME, APR_READ, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    if (rv  == APR_INCOMPLETE) {
        char *str;
	int i;
        str = apr_pstrdup(p, "APR_INCOMPLETE:  Missing ");
        for (i = 0; vfi[i].bits; ++i) {
            if (vfi[i].bits & ~finfo.valid) {
                str = apr_pstrcat(p, str, vfi[i].description, " ", NULL);
            }
        }
        CuFail(tc, str);
    }
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    apr_file_close(thefile);
}

static void test_stat(CuTest *tc)
{
    apr_finfo_t finfo;
    apr_status_t rv;

    rv = apr_stat(&finfo, FILENAME, APR_FINFO_NORM, p);
    if (rv  == APR_INCOMPLETE) {
        char *str;
	int i;
        str = apr_pstrdup(p, "APR_INCOMPLETE:  Missing ");
        for (i = 0; vfi[i].bits; ++i) {
            if (vfi[i].bits & ~finfo.valid) {
                str = apr_pstrcat(p, str, vfi[i].description, " ", NULL);
            }
        }
        CuFail(tc, str);
    }
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_stat_eq_finfo(CuTest *tc)
{
    apr_file_t *thefile;
    apr_finfo_t finfo;
    apr_finfo_t stat_finfo;
    apr_status_t rv;

    rv = apr_file_open(&thefile, FILENAME, APR_READ, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);

    /* Opening the file may have toggled the atime member (time last
     * accessed), so fetch our apr_stat() after getting the fileinfo 
     * of the open file...
     */
    rv = apr_stat(&stat_finfo, FILENAME, APR_FINFO_NORM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    apr_file_close(thefile);

    finfo_equal(tc, stat_finfo, finfo);
}

static void test_buffered_write_size(CuTest *tc)
{
    const apr_size_t data_len = strlen(NEWFILEDATA);
    apr_file_t *thefile;
    apr_finfo_t finfo;
    apr_status_t rv;
    apr_size_t bytes;

    rv = apr_file_open(&thefile, NEWFILENAME,
                       APR_READ | APR_WRITE | APR_CREATE | APR_TRUNCATE
                       | APR_BUFFERED | APR_DELONCLOSE,
                       APR_OS_DEFAULT, p);
    apr_assert_success(tc, "open file", rv);

    /* A funny thing happened to me the other day: I wrote something
     * into a buffered file, then asked for its size using
     * apr_file_info_get; and guess what? The size was 0! That's not a
     * nice way to behave.
     */
    bytes = data_len;
    rv = apr_file_write(thefile, NEWFILEDATA, &bytes);
    apr_assert_success(tc, "write file contents", rv);
    CuAssertTrue(tc, data_len == bytes);

    rv = apr_file_info_get(&finfo, APR_FINFO_SIZE, thefile);
    apr_assert_success(tc, "get file size", rv);
    CuAssertTrue(tc, bytes == (apr_size_t) finfo.size);
    apr_file_close(thefile);
}

static void test_mtime_set(CuTest *tc)
{
    apr_file_t *thefile;
    apr_finfo_t finfo;
    apr_time_t epoch = 0;
    apr_status_t rv;

    /* This test sort of depends on the system clock being at least
     * marginally ccorrect; We'll be setting the modification time to
     * the epoch.
     */
    rv = apr_file_open(&thefile, NEWFILENAME,
                       APR_READ | APR_WRITE | APR_CREATE | APR_TRUNCATE
                       | APR_BUFFERED | APR_DELONCLOSE,
                       APR_OS_DEFAULT, p);
    apr_assert_success(tc, "open file", rv);

    /* Check that the current mtime is not the epoch */
    rv = apr_stat(&finfo, NEWFILENAME, APR_FINFO_MTIME, p);
    if (rv  == APR_INCOMPLETE) {
        char *str;
	int i;
        str = apr_pstrdup(p, "APR_INCOMPLETE:  Missing ");
        for (i = 0; vfi[i].bits; ++i) {
            if (vfi[i].bits & ~finfo.valid) {
                str = apr_pstrcat(p, str, vfi[i].description, " ", NULL);
            }
        }
        CuFail(tc, str);
    }
    apr_assert_success(tc, "get initial mtime", rv);
    CuAssertTrue(tc, finfo.mtime != epoch);

    /* Reset the mtime to the epoch and verify the result.
     * Note: we blindly assume that if the first apr_stat succeeded,
     * the second one will, too.
     */
    rv = apr_file_mtime_set(NEWFILENAME, epoch, p);
    apr_assert_success(tc, "set mtime", rv);

    rv = apr_stat(&finfo, NEWFILENAME, APR_FINFO_MTIME, p);
    apr_assert_success(tc, "get modified mtime", rv);
    CuAssertTrue(tc, finfo.mtime == epoch);

    apr_file_close(thefile);
}

CuSuite *testfileinfo(void)
{
    CuSuite *suite = CuSuiteNew("File Info");

    SUITE_ADD_TEST(suite, test_info_get);
    SUITE_ADD_TEST(suite, test_stat);
    SUITE_ADD_TEST(suite, test_stat_eq_finfo);
    SUITE_ADD_TEST(suite, test_buffered_write_size);
    SUITE_ADD_TEST(suite, test_mtime_set);

    return suite;
}

