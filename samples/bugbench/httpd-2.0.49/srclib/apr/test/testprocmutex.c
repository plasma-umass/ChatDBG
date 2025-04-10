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

#include "apr_shm.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_proc_mutex.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"

#if APR_HAS_FORK

#define MAX_ITER 200
#define CHILDREN 6
#define MAX_COUNTER (MAX_ITER * CHILDREN)

static apr_proc_mutex_t *proc_lock;
static volatile int *x;

/* a slower more racy way to implement (*x)++ */
static int increment(int n)
{
    apr_sleep(1);
    return n+1;
}

static void make_child(CuTest *tc, apr_proc_t **proc, apr_pool_t *p)
{
    apr_status_t rv;

    *proc = apr_pcalloc(p, sizeof(**proc));

    /* slight delay to allow things to settle */
    apr_sleep (1);

    rv = apr_proc_fork(*proc, p);
    if (rv == APR_INCHILD) {
        int i = 0;
        /* The parent process has setup all processes to call apr_terminate
         * at exit.  But, that means that all processes must also call
         * apr_initialize at startup.  You cannot have an unequal number
         * of apr_terminate and apr_initialize calls.  If you do, bad things
         * will happen.  In this case, the bad thing is that if the mutex
         * is a semaphore, it will be destroyed before all of the processes
         * die.  That means that the test will most likely fail.
         */
        apr_initialize();

        if (apr_proc_mutex_child_init(&proc_lock, NULL, p))
            exit(1);

        do {
            if (apr_proc_mutex_lock(proc_lock))
                exit(1);
            i++;
            *x = increment(*x);
            if (apr_proc_mutex_unlock(proc_lock))
                exit(1);
        } while (i < MAX_ITER);
        exit(0);
    } 

    CuAssert(tc, "fork failed", rv == APR_INPARENT);
}

/* Wait for a child process and check it terminated with success. */
static void await_child(CuTest *tc, apr_proc_t *proc)
{
    int code;
    apr_exit_why_e why;
    apr_status_t rv;

    rv = apr_proc_wait(proc, &code, &why, APR_WAIT);
    CuAssert(tc, "child did not terminate with success",
             rv == APR_CHILD_DONE && why == APR_PROC_EXIT && code == 0);
}

static void test_exclusive(CuTest *tc, const char *lockname)
{
    apr_proc_t *child[CHILDREN];
    apr_status_t rv;
    int n;
 
    rv = apr_proc_mutex_create(&proc_lock, lockname, APR_LOCK_DEFAULT, p);
    apr_assert_success(tc, "create the mutex", rv);
 
    for (n = 0; n < CHILDREN; n++)
        make_child(tc, &child[n], p);

    for (n = 0; n < CHILDREN; n++)
        await_child(tc, child[n]);
    
    CuAssert(tc, "Locks don't appear to work", *x == MAX_COUNTER);
}
#endif

static void proc_mutex(CuTest *tc)
{
#if APR_HAS_FORK
    apr_status_t rv;
    const char *shmname = "tpm.shm";
    apr_shm_t *shm;

    /* Use anonymous shm if available. */
    rv = apr_shm_create(&shm, sizeof(int), NULL, p);
    if (rv == APR_ENOTIMPL) {
        apr_file_remove(shmname, p);
        rv = apr_shm_create(&shm, sizeof(int), shmname, p);
    }

    apr_assert_success(tc, "create shm segment", rv);

    x = apr_shm_baseaddr_get(shm);
    test_exclusive(tc, NULL);
#else
    CuNotImpl(tc, "APR lacks fork() support");
#endif
}


CuSuite *testprocmutex(void)
{
    CuSuite *suite = CuSuiteNew("Cross-Process Mutexes");

    SUITE_ADD_TEST(suite, proc_mutex);

    return suite;
}

