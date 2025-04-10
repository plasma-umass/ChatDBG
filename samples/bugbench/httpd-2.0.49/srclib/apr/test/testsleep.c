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

#include "time.h"
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"

#define SLEEP_INTERVAL 5

static void sleep_one(CuTest *tc)
{
    time_t pretime = time(NULL);
    time_t posttime;
    time_t timediff;

    apr_sleep(apr_time_from_sec(SLEEP_INTERVAL));
    posttime = time(NULL);

    /* normalize the timediff.  We should have slept for SLEEP_INTERVAL, so
     * we should just subtract that out.
     */
    timediff = posttime - pretime - SLEEP_INTERVAL;
    CuAssertTrue(tc, timediff >= 0);
    CuAssertTrue(tc, timediff <= 1);
}

CuSuite *testsleep(void)
{
    CuSuite *suite = CuSuiteNew("Sleep");

    SUITE_ADD_TEST(suite, sleep_one);

    return suite;
}

