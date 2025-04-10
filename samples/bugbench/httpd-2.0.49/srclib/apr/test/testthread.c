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

#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include "apr_time.h"
#include "test_apr.h"

#if APR_HAS_THREADS

static apr_thread_mutex_t *thread_lock;
static apr_thread_once_t *control = NULL;
static int x = 0;
static int value = 0;

static apr_thread_t *t1;
static apr_thread_t *t2;
static apr_thread_t *t3;
static apr_thread_t *t4;

/* just some made up number to check on later */
static apr_status_t exit_ret_val = 123;

static void init_func(void)
{
    value++;
}

static void * APR_THREAD_FUNC thread_func1(apr_thread_t *thd, void *data)
{
    int i;

    apr_thread_once(control, init_func);

    for (i = 0; i < 10000; i++) {
        apr_thread_mutex_lock(thread_lock);
        x++;
        apr_thread_mutex_unlock(thread_lock);
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
} 

static void thread_init(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_thread_once_init(&control, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_thread_mutex_create(&thread_lock, APR_THREAD_MUTEX_DEFAULT, p); 
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void create_threads(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_thread_create(&t1, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_thread_create(&t2, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_thread_create(&t3, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_thread_create(&t4, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void join_threads(CuTest *tc)
{
    apr_status_t s;

    apr_thread_join(&s, t1);
    CuAssertIntEquals(tc, exit_ret_val, s);
    apr_thread_join(&s, t2);
    CuAssertIntEquals(tc, exit_ret_val, s);
    apr_thread_join(&s, t3);
    CuAssertIntEquals(tc, exit_ret_val, s);
    apr_thread_join(&s, t4);
    CuAssertIntEquals(tc, exit_ret_val, s);
}

static void check_locks(CuTest *tc)
{
    CuAssertIntEquals(tc, 40000, x);
}

static void check_thread_once(CuTest *tc)
{
    CuAssertIntEquals(tc, 1, value);
}

#else

static void threads_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Threads not implemented on this platform");
}

#endif

CuSuite *testthread(void)
{
    CuSuite *suite = CuSuiteNew("Threads");

#if !APR_HAS_THREADS
    SUITE_ADD_TEST(suite, threads_not_impl);
#else
    SUITE_ADD_TEST(suite, thread_init);
    SUITE_ADD_TEST(suite, create_threads);
    SUITE_ADD_TEST(suite, join_threads);
    SUITE_ADD_TEST(suite, check_locks);
    SUITE_ADD_TEST(suite, check_thread_once);
#endif

    return suite;
}

