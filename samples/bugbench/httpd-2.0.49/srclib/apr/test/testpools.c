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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "test_apr.h"

#define ALLOC_BYTES 1024

static apr_pool_t *pmain = NULL;
static apr_pool_t *pchild = NULL;

static void alloc_bytes(CuTest *tc)
{
    int i;
    char *alloc;
    
    alloc = apr_palloc(pmain, ALLOC_BYTES);
    CuAssertPtrNotNull(tc, alloc);

    for (i=0;i<ALLOC_BYTES;i++) {
        char *ptr = alloc + i;
        *ptr = 0xa;
    }
    /* This is just added to get the positive.  If this test fails, the
     * suite will seg fault.
     */
    CuAssertTrue(tc, 1);
}

static void calloc_bytes(CuTest *tc)
{
    int i;
    char *alloc;
    
    alloc = apr_pcalloc(pmain, ALLOC_BYTES);
    CuAssertPtrNotNull(tc, alloc);

    for (i=0;i<ALLOC_BYTES;i++) {
        char *ptr = alloc + i;
        CuAssertTrue(tc, *ptr == '\0');
    }
}

static void parent_pool(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_pool_create(&pmain, NULL);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrNotNull(tc, pmain);
}

static void child_pool(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_pool_create(&pchild, pmain);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrNotNull(tc, pchild);
}

static void test_ancestor(CuTest *tc)
{
    CuAssertIntEquals(tc, 1, apr_pool_is_ancestor(pmain, pchild));
}

static void test_notancestor(CuTest *tc)
{
    CuAssertIntEquals(tc, 0, apr_pool_is_ancestor(pchild, pmain));
}

CuSuite *testpool(void)
{
    CuSuite *suite = CuSuiteNew("Pools");

    SUITE_ADD_TEST(suite, parent_pool);
    SUITE_ADD_TEST(suite, child_pool);
    SUITE_ADD_TEST(suite, test_ancestor);
    SUITE_ADD_TEST(suite, test_notancestor);
    SUITE_ADD_TEST(suite, alloc_bytes);
    SUITE_ADD_TEST(suite, calloc_bytes);

    return suite;
}

