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
#include "apr_file_io.h"
#include "apr_thread_mutex.h"
#include "apr_thread_rwlock.h"
#include "apr_thread_cond.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "test_apr.h"

#if APR_HAS_THREADS

#define MAX_ITER 40000
#define MAX_COUNTER 100000
#define MAX_RETRY 5

static void *APR_THREAD_FUNC thread_rwlock_func(apr_thread_t *thd, void *data);
static void *APR_THREAD_FUNC thread_mutex_function(apr_thread_t *thd, void *data);
static void *APR_THREAD_FUNC thread_cond_producer(apr_thread_t *thd, void *data);
static void *APR_THREAD_FUNC thread_cond_consumer(apr_thread_t *thd, void *data);

static apr_thread_mutex_t *thread_mutex;
static apr_thread_rwlock_t *rwlock;
static int i = 0, x = 0;

static int buff[MAX_COUNTER];

struct {
    apr_thread_mutex_t *mutex;
    int                nput;
    int                nval;
} put;

struct {
    apr_thread_mutex_t *mutex;
    apr_thread_cond_t  *cond;
    int                nready;
} nready;

static apr_thread_mutex_t *timeout_mutex;
static apr_thread_cond_t *timeout_cond;

static void *APR_THREAD_FUNC thread_rwlock_func(apr_thread_t *thd, void *data)
{
    int exitLoop = 1;

    while (1)
    {
        apr_thread_rwlock_rdlock(rwlock);
        if (i == MAX_ITER)
            exitLoop = 0;
        apr_thread_rwlock_unlock(rwlock);

        if (!exitLoop)
            break;

        apr_thread_rwlock_wrlock(rwlock);
        if (i != MAX_ITER)
        {
            i++;
            x++;
        }
        apr_thread_rwlock_unlock(rwlock);
    }
    return NULL;
} 

static void *APR_THREAD_FUNC thread_mutex_function(apr_thread_t *thd, void *data)
{
    int exitLoop = 1;

    /* slight delay to allow things to settle */
    apr_sleep (1);
    
    while (1)
    {
        apr_thread_mutex_lock(thread_mutex);
        if (i == MAX_ITER)
            exitLoop = 0;
        else 
        {
            i++;
            x++;
        }
        apr_thread_mutex_unlock(thread_mutex);

        if (!exitLoop)
            break;
    }
    return NULL;
} 

static void *APR_THREAD_FUNC thread_cond_producer(apr_thread_t *thd, void *data)
{
    for (;;) {
        apr_thread_mutex_lock(put.mutex);
        if (put.nput >= MAX_COUNTER) {
            apr_thread_mutex_unlock(put.mutex);
            return NULL;
        }
        buff[put.nput] = put.nval;
        put.nput++;
        put.nval++;
        apr_thread_mutex_unlock(put.mutex);

        apr_thread_mutex_lock(nready.mutex);
        if (nready.nready == 0)
            apr_thread_cond_signal(nready.cond);
        nready.nready++;
        apr_thread_mutex_unlock(nready.mutex);

        *((int *) data) += 1;
    }

    return NULL;
}

static void *APR_THREAD_FUNC thread_cond_consumer(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < MAX_COUNTER; i++) {
        apr_thread_mutex_lock(nready.mutex);
        while (nready.nready == 0)
            apr_thread_cond_wait(nready.cond, nready.mutex);
        nready.nready--;
        apr_thread_mutex_unlock(nready.mutex);

        if (buff[i] != i)
            printf("buff[%d] = %d\n", i, buff[i]);
    }

    return NULL;
}

static void test_thread_mutex(CuTest *tc)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    s1 = apr_thread_mutex_create(&thread_mutex, APR_THREAD_MUTEX_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s1);
    CuAssertPtrNotNull(tc, thread_mutex);

    i = 0;
    x = 0;

    s1 = apr_thread_create(&t1, NULL, thread_mutex_function, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s1);
    s2 = apr_thread_create(&t2, NULL, thread_mutex_function, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s2);
    s3 = apr_thread_create(&t3, NULL, thread_mutex_function, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s3);
    s4 = apr_thread_create(&t4, NULL, thread_mutex_function, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s4);

    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);

    CuAssertIntEquals(tc, MAX_ITER, x);
}

static void test_thread_rwlock(CuTest *tc)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    s1 = apr_thread_rwlock_create(&rwlock, p);
    apr_assert_success(tc, "rwlock_create", s1);
    CuAssertPtrNotNull(tc, rwlock);

    i = 0;
    x = 0;

    s1 = apr_thread_create(&t1, NULL, thread_rwlock_func, NULL, p);
    apr_assert_success(tc, "create thread 1", s1);
    s2 = apr_thread_create(&t2, NULL, thread_rwlock_func, NULL, p);
    apr_assert_success(tc, "create thread 2", s2);
    s3 = apr_thread_create(&t3, NULL, thread_rwlock_func, NULL, p);
    apr_assert_success(tc, "create thread 3", s3);
    s4 = apr_thread_create(&t4, NULL, thread_rwlock_func, NULL, p);
    apr_assert_success(tc, "create thread 4", s4);

    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);

    CuAssertIntEquals(tc, MAX_ITER, x);
}

static void test_cond(CuTest *tc)
{
    apr_thread_t *p1, *p2, *p3, *p4, *c1;
    apr_status_t s0, s1, s2, s3, s4;
    int count1, count2, count3, count4;
    int sum;

    s1 = apr_thread_mutex_create(&put.mutex, APR_THREAD_MUTEX_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s1);
    CuAssertPtrNotNull(tc, put.mutex);

    s1 = apr_thread_mutex_create(&nready.mutex, APR_THREAD_MUTEX_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s1);
    CuAssertPtrNotNull(tc, nready.mutex);

    s1 = apr_thread_cond_create(&nready.cond, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s1);
    CuAssertPtrNotNull(tc, nready.cond);

    count1 = count2 = count3 = count4 = 0;
    put.nput = put.nval = 0;
    nready.nready = 0;
    i = 0;
    x = 0;

    s0 = apr_thread_create(&p1, NULL, thread_cond_producer, &count1, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s0);
    s1 = apr_thread_create(&p2, NULL, thread_cond_producer, &count2, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s1);
    s2 = apr_thread_create(&p3, NULL, thread_cond_producer, &count3, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s2);
    s3 = apr_thread_create(&p4, NULL, thread_cond_producer, &count4, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s3);
    s4 = apr_thread_create(&c1, NULL, thread_cond_consumer, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s4);

    apr_thread_join(&s0, p1);
    apr_thread_join(&s1, p2);
    apr_thread_join(&s2, p3);
    apr_thread_join(&s3, p4);
    apr_thread_join(&s4, c1);

    sum = count1 + count2 + count3 + count4;
    /*
    printf("count1 = %d count2 = %d count3 = %d count4 = %d\n",
            count1, count2, count3, count4);
    */
    CuAssertIntEquals(tc, MAX_COUNTER, sum);
}

static void test_timeoutcond(CuTest *tc)
{
    apr_status_t s;
    apr_interval_time_t timeout;
    apr_time_t begin, end;
    int i;

    s = apr_thread_mutex_create(&timeout_mutex, APR_THREAD_MUTEX_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertPtrNotNull(tc, timeout_mutex);

    s = apr_thread_cond_create(&timeout_cond, p);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertPtrNotNull(tc, timeout_cond);

    timeout = apr_time_from_sec(5);

    for (i = 0; i < MAX_RETRY; i++) {
        apr_thread_mutex_lock(timeout_mutex);

        begin = apr_time_now();
        s = apr_thread_cond_timedwait(timeout_cond, timeout_mutex, timeout);
        end = apr_time_now();
        apr_thread_mutex_unlock(timeout_mutex);
        
        if (s != APR_SUCCESS && !APR_STATUS_IS_TIMEUP(s)) {
            continue;
        }
        CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(s));
        CuAssert(tc, "Timer returned too late", end - begin - timeout < 100000);
        break;
    }
    CuAssert(tc, "Too many retries", i < MAX_RETRY);
}

#endif /* !APR_HAS_THREADS */

#if !APR_HAS_THREADS
static void threads_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Threads not implemented on this platform");
}
#endif


CuSuite *testlock(void)
{
    CuSuite *suite = CuSuiteNew("Thread Locks");

#if !APR_HAS_THREADS
    SUITE_ADD_TEST(suite, threads_not_impl);
#else
    SUITE_ADD_TEST(suite, test_thread_mutex);
    SUITE_ADD_TEST(suite, test_thread_rwlock);
    SUITE_ADD_TEST(suite, test_cond);
    SUITE_ADD_TEST(suite, test_timeoutcond);
#endif

    return suite;
}

