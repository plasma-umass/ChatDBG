/* Copyright 2002-2004 The Apache Software Foundation
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
#include "apr.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_tables.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

static apr_table_t *t1 = NULL;

static void table_make(CuTest *tc)
{
    t1 = apr_table_make(p, 5);
    CuAssertPtrNotNull(tc, t1);
}

static void table_get(CuTest *tc)
{
    const char *val;

    apr_table_set(t1, "foo", "bar");
    val = apr_table_get(t1, "foo");
    CuAssertStrEquals(tc, val, "bar");
}

static void table_set(CuTest *tc)
{
    const char *val;

    apr_table_set(t1, "setkey", "bar");
    apr_table_set(t1, "setkey", "2ndtry");
    val = apr_table_get(t1, "setkey");
    CuAssertStrEquals(tc, val, "2ndtry");
}

static void table_getnotthere(CuTest *tc)
{
    const char *val;

    val = apr_table_get(t1, "keynotthere");
    CuAssertPtrEquals(tc, NULL, (void *)val);
}

static void table_add(CuTest *tc)
{
    const char *val;

    apr_table_add(t1, "addkey", "bar");
    apr_table_add(t1, "addkey", "foo");
    val = apr_table_get(t1, "addkey");
    CuAssertStrEquals(tc, val, "bar");

}

static void table_nelts(CuTest *tc)
{
    const char *val;
    apr_table_t *t = apr_table_make(p, 1);

    apr_table_set(t, "abc", "def");
    apr_table_set(t, "def", "abc");
    apr_table_set(t, "foo", "zzz");
    val = apr_table_get(t, "foo");
    CuAssertStrEquals(tc, val, "zzz");
    val = apr_table_get(t, "abc");
    CuAssertStrEquals(tc, val, "def");
    val = apr_table_get(t, "def");
    CuAssertStrEquals(tc, val, "abc");
    CuAssertIntEquals(tc, 3, apr_table_elts(t)->nelts);
}

static void table_clear(CuTest *tc)
{
    apr_table_clear(t1);
    CuAssertIntEquals(tc, 0, apr_table_elts(t1)->nelts);
}

static void table_unset(CuTest *tc)
{
    const char *val;
    apr_table_t *t = apr_table_make(p, 1);

    apr_table_set(t, "a", "1");
    apr_table_set(t, "b", "2");
    apr_table_unset(t, "b");
    CuAssertIntEquals(tc, 1, apr_table_elts(t)->nelts);
    val = apr_table_get(t, "a");
    CuAssertStrEquals(tc, val, "1");
    val = apr_table_get(t, "b");
    CuAssertPtrEquals(tc, (void *)val, (void *)NULL);
}

static void table_overlap(CuTest *tc)
{
    const char *val;
    apr_table_t *t1 = apr_table_make(p, 1);
    apr_table_t *t2 = apr_table_make(p, 1);

    apr_table_addn(t1, "a", "0");
    apr_table_addn(t1, "g", "7");
    apr_table_addn(t2, "a", "1");
    apr_table_addn(t2, "b", "2");
    apr_table_addn(t2, "c", "3");
    apr_table_addn(t2, "b", "2.0");
    apr_table_addn(t2, "d", "4");
    apr_table_addn(t2, "e", "5");
    apr_table_addn(t2, "b", "2.");
    apr_table_addn(t2, "f", "6");
    apr_table_overlap(t1, t2, APR_OVERLAP_TABLES_SET);
    
    CuAssertIntEquals(tc, apr_table_elts(t1)->nelts, 7);
    val = apr_table_get(t1, "a");
    CuAssertStrEquals(tc, val, "1");
    val = apr_table_get(t1, "b");
    CuAssertStrEquals(tc, val, "2.");
    val = apr_table_get(t1, "c");
    CuAssertStrEquals(tc, val, "3");
    val = apr_table_get(t1, "d");
    CuAssertStrEquals(tc, val, "4");
    val = apr_table_get(t1, "e");
    CuAssertStrEquals(tc, val, "5");
    val = apr_table_get(t1, "f");
    CuAssertStrEquals(tc, val, "6");
    val = apr_table_get(t1, "g");
    CuAssertStrEquals(tc, val, "7");
}

CuSuite *testtable(void)
{
    CuSuite *suite = CuSuiteNew("Table");

    SUITE_ADD_TEST(suite, table_make);
    SUITE_ADD_TEST(suite, table_get);
    SUITE_ADD_TEST(suite, table_set);
    SUITE_ADD_TEST(suite, table_getnotthere);
    SUITE_ADD_TEST(suite, table_add);
    SUITE_ADD_TEST(suite, table_nelts);
    SUITE_ADD_TEST(suite, table_clear);
    SUITE_ADD_TEST(suite, table_unset);
    SUITE_ADD_TEST(suite, table_overlap);

    return suite;
}

