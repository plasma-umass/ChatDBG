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

#include "test_apr.h"
#include "apr_version.h"
#include "apr_general.h"


static void test_strings(CuTest *tc)
{
    CuAssertStrEquals(tc, APR_VERSION_STRING, apr_version_string());
}

static void test_ints(CuTest *tc)
{
    apr_version_t vsn;

    apr_version(&vsn);

    CuAssertIntEquals(tc, APR_MAJOR_VERSION, vsn.major);
    CuAssertIntEquals(tc, APR_MINOR_VERSION, vsn.minor);
    CuAssertIntEquals(tc, APR_PATCH_VERSION, vsn.patch);
}

CuSuite *testvsn(void)
{
    CuSuite *suite = CuSuiteNew("Versioning");

    SUITE_ADD_TEST(suite, test_strings);
    SUITE_ADD_TEST(suite, test_ints);

    return suite;
}

