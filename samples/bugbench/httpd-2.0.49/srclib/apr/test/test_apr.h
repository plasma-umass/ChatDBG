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

#ifndef APR_TEST_INCLUDES
#define APR_TEST_INCLUDES

#include "CuTest.h"
#include "apr_pools.h"

/* Some simple functions to make the test apps easier to write and
 * a bit more consistent...
 */

extern apr_pool_t *p;

CuSuite *getsuite(void);

CuSuite *teststr(void);
CuSuite *testtime(void);
CuSuite *testvsn(void);
CuSuite *testipsub(void);
CuSuite *testmmap(void);
CuSuite *testud(void);
CuSuite *testtable(void);
CuSuite *testhash(void);
CuSuite *testsleep(void);
CuSuite *testpool(void);
CuSuite *testfmt(void);
CuSuite *testfile(void);
CuSuite *testdir(void);
CuSuite *testfileinfo(void);
CuSuite *testrand(void);
CuSuite *testdso(void);
CuSuite *testoc(void);
CuSuite *testdup(void);
CuSuite *testsockets(void);
CuSuite *testproc(void);
CuSuite *testprocmutex(void);
CuSuite *testpoll(void);
CuSuite *testlock(void);
CuSuite *testsockopt(void);
CuSuite *testpipe(void);
CuSuite *testthread(void);
CuSuite *testgetopt(void);
CuSuite *testnames(void);
CuSuite *testuser(void);
CuSuite *testpath(void);
CuSuite *testenv(void);

/* Assert that RV is an APR_SUCCESS value; else fail giving strerror
 * for RV and CONTEXT message. */
void apr_assert_success(CuTest* tc, const char *context, apr_status_t rv);


#endif /* APR_TEST_INCLUDES */
