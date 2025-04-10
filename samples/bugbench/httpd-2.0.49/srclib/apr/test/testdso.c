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
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_dso.h"
#include "apr_strings.h"
#include "apr.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef NETWARE
# define MOD_NAME "mod_test.nlm"
#elif defined(BEOS) || defined(WIN32)
# define MOD_NAME "mod_test.so"
#elif defined(DARWIN)
# define MOD_NAME ".libs/mod_test.so" 
# define LIB_NAME ".libs/libmod_test.dylib" 
#elif defined(__hpux__) || defined(__hpux)
# define MOD_NAME ".libs/mod_test.sl"
# define LIB_NAME ".libs/libmod_test.sl"
#elif defined(_AIX) || defined(__bsdi__)
# define MOD_NAME ".libs/libmod_test.so"
# define LIB_NAME ".libs/libmod_test.so"
#else /* Every other Unix */
# define MOD_NAME ".libs/mod_test.so"
# define LIB_NAME ".libs/libmod_test.so"
#endif

static char *modname;

static void test_load_module(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    apr_dso_unload(h);
}

static void test_dso_sym(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    void (*function)(char str[256]);
    char teststr[256];
    char errstr[256];

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (void (*)(char *))func1;
    (*function)(teststr);
    CuAssertStrEquals(tc, "Hello - I'm a DSO!\n", teststr);

    apr_dso_unload(h);
}

static void test_dso_sym_return_value(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    int (*function)(int);
    char errstr[256];

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "count_reps");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (int (*)(int))func1;
    status = (*function)(5);
    CuAssertIntEquals(tc, 5, status);

    apr_dso_unload(h);
}

static void test_unload_module(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];
    apr_dso_handle_sym_t func1 = NULL;

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_unload(h);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ESYMNOTFOUND(status));
}


#ifdef LIB_NAME
static char *libname;

static void test_load_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    apr_dso_unload(h);
}

static void test_dso_sym_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    void (*function)(char str[256]);
    char teststr[256];
    char errstr[256];

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (void (*)(char *))func1;
    (*function)(teststr);
    CuAssertStrEquals(tc, "Hello - I'm a DSO!\n", teststr);

    apr_dso_unload(h);
}

static void test_dso_sym_return_value_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    int (*function)(int);
    char errstr[256];

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "count_reps");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (int (*)(int))func1;
    status = (*function)(5);
    CuAssertIntEquals(tc, 5, status);

    apr_dso_unload(h);
}

static void test_unload_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];
    apr_dso_handle_sym_t func1 = NULL;

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_unload(h);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ESYMNOTFOUND(status));
}

#endif /* def(LIB_NAME) */

static void test_load_notthere(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;

    status = apr_dso_load(&h, "No_File.so", p);

    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EDSOOPEN(status));
    CuAssertPtrNotNull(tc, h);
}    

CuSuite *testdso(void)
{
    CuSuite *suite = CuSuiteNew("DSO");

    modname = apr_pcalloc(p, 256);
    getcwd(modname, 256);
    modname = apr_pstrcat(p, modname, "/", MOD_NAME, NULL);

    SUITE_ADD_TEST(suite, test_load_module);
    SUITE_ADD_TEST(suite, test_dso_sym);
    SUITE_ADD_TEST(suite, test_dso_sym_return_value);
    SUITE_ADD_TEST(suite, test_unload_module);

#ifdef LIB_NAME
    libname = apr_pcalloc(p, 256);
    getcwd(libname, 256);
    libname = apr_pstrcat(p, libname, "/", LIB_NAME, NULL);

    SUITE_ADD_TEST(suite, test_load_library);
    SUITE_ADD_TEST(suite, test_dso_sym_library);
    SUITE_ADD_TEST(suite, test_dso_sym_return_value_library);
    SUITE_ADD_TEST(suite, test_unload_library);
#endif

    SUITE_ADD_TEST(suite, test_load_notthere);

    return suite;
}

