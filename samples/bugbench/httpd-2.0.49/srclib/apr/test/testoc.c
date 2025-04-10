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
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"

#if APR_HAS_OTHER_CHILD

/* XXX I'm sure there has to be a better way to do this ... */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif

static char reasonstr[256];

static void ocmaint(int reason, void *data, int status)
{
    switch (reason) {
    case APR_OC_REASON_DEATH:
        apr_cpystrn(reasonstr, "APR_OC_REASON_DEATH", 
                    strlen("APR_OC_REASON_DEATH") + 1);
        break;
    case APR_OC_REASON_LOST:
        apr_cpystrn(reasonstr, "APR_OC_REASON_LOST", 
                    strlen("APR_OC_REASON_LOST") + 1);
        break;
    case APR_OC_REASON_UNWRITABLE:
        apr_cpystrn(reasonstr, "APR_OC_REASON_UNWRITEABLE", 
                    strlen("APR_OC_REASON_UNWRITEABLE") + 1);
        break;
    case APR_OC_REASON_RESTART:
        apr_cpystrn(reasonstr, "APR_OC_REASON_RESTART", 
                    strlen("APR_OC_REASON_RESTART") + 1);
        break;
    }
}

#ifndef SIGKILL
#define SIGKILL 1
#endif

/* It would be great if we could stress this stuff more, and make the test
 * more granular.
 */
static void test_child_kill(CuTest *tc)
{
    apr_file_t *std = NULL;
    apr_proc_t newproc;
    apr_procattr_t *procattr = NULL;
    const char *args[3];
    apr_status_t rv;

    args[0] = apr_pstrdup(p, "occhild" EXTENSION);
    args[1] = apr_pstrdup(p, "-X");
    args[2] = NULL;

    rv = apr_procattr_create(&procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_io_set(procattr, APR_FULL_BLOCK, APR_NO_PIPE, 
                             APR_NO_PIPE);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_proc_create(&newproc, "./occhild" EXTENSION, args, NULL, procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, newproc.in);
    CuAssertPtrEquals(tc, NULL, newproc.out);
    CuAssertPtrEquals(tc, NULL, newproc.err);

    std = newproc.in;

    apr_proc_other_child_register(&newproc, ocmaint, NULL, std, p);

    apr_sleep(apr_time_from_sec(1));
    rv = apr_proc_kill(&newproc, SIGKILL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    /* allow time for things to settle... */
    apr_sleep(apr_time_from_sec(3));
    
    apr_proc_other_child_check();
    CuAssertStrEquals(tc, "APR_OC_REASON_DEATH", reasonstr);
}    
#else

static void oc_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Other child logic not implemented on this platform");
}
#endif

CuSuite *testoc(void)
{
    CuSuite *suite = CuSuiteNew("Other Child");

#if !APR_HAS_OTHER_CHILD
    SUITE_ADD_TEST(suite, oc_not_impl);
#else

    SUITE_ADD_TEST(suite, test_child_kill); 

#endif
    return suite;
}

