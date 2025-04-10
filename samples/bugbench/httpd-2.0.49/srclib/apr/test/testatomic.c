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
#include <stdlib.h>
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_atomic.h"
#include "errno.h"
#include "apr_time.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#if !(defined WIN32) && !(defined NETWARE)
#include <pthread.h>
#endif

apr_pool_t *context;
apr_atomic_t y;      /* atomic locks */

static apr_status_t check_basic_atomics(volatile apr_atomic_t*p)
{
    apr_atomic_t oldval;
    apr_uint32_t casval = 0;
    float object1, object2;
    void *casptr;
    void *oldptr;

    apr_atomic_set(&y, 2);
    printf("%-60s", "testing apr_atomic_dec");
    if (apr_atomic_dec(&y) == 0) {
        fprintf(stderr, "Failed\noldval =%d should not be zero\n",
                apr_atomic_read(&y));
        return APR_EGENERAL;
    }
    if (apr_atomic_dec(&y) != 0) {
        fprintf(stderr, "Failed\noldval =%d should be zero\n",
                apr_atomic_read(&y));
        return APR_EGENERAL;
    }
    printf("OK\n");

    printf("%-60s", "testing CAS");
    oldval = apr_atomic_cas(&casval, 12, 0);
    if (oldval != 0) {
        fprintf(stderr, "Failed\noldval =%d should be zero\n", oldval);
        return APR_EGENERAL;
    }
    printf("OK\n");
    printf("%-60s", "testing CAS - match non-null");
    oldval = apr_atomic_cas(&casval, 23, 12);
    if (oldval != 12) {
        fprintf(stderr, "Failed\noldval =%d should be 12 y=%d\n",
                oldval, casval);
        return APR_EGENERAL;
    }
    printf("OK\n");
    printf("%-60s", "testing CAS - no match");
    oldval = apr_atomic_cas(&casval, 23, 12);
    if (oldval != 23) {
        fprintf(stderr, "Failed\noldval =%d should be 23 y=%d\n",
                oldval, casval);
        return APR_EGENERAL;
    }
    printf("OK\n");

    printf("%-60s", "testing CAS for pointers");
    casptr = NULL;
    oldptr = apr_atomic_casptr(&casptr, &object1, 0);
    if (oldptr != 0) {
        fprintf(stderr, "Failed\noldval =%p should be zero\n", oldptr);
        return APR_EGENERAL;
    }
    printf("OK\n");
    printf("%-60s", "testing CAS for pointers - match non-null");
    oldptr = apr_atomic_casptr(&casptr, &object2, &object1);
    if (oldptr != &object1) {
        fprintf(stderr, "Failed\noldval =%p should be %p\n", oldptr, &object1);
        return APR_EGENERAL;
    }
    printf("OK\n");
    printf("%-60s", "testing CAS for pointers - no match");
    oldptr = apr_atomic_casptr(&casptr, &object2, &object1);
    if (oldptr != &object2) {
        fprintf(stderr, "Failed\noldval =%p should be %p\n", oldptr, &object2);
        return APR_EGENERAL;
    }
    printf("OK\n");

    printf("%-60s", "testing add");
    apr_atomic_set(&y, 23);
    apr_atomic_add(&y, 4);
    if (apr_atomic_read(&y) != 27) {
        fprintf(stderr,
                "Failed\nAtomic Add doesn't add up ;( expected 27 got %d\n",
                oldval);
        return APR_EGENERAL;
    }
 
    printf("OK\n");
    printf("%-60s", "testing add/inc");
    apr_atomic_set(&y, 0);
    apr_atomic_add(&y, 20);
    apr_atomic_inc(&y);
    if (apr_atomic_read(&y) != 21) {
        fprintf(stderr, "Failed.\natomics do not add up\n");
        return APR_EGENERAL;
    }
    fprintf(stdout, "OK\n");

    return APR_SUCCESS;
}

#if !APR_HAS_THREADS
int main(void)
{
    apr_status_t rv;

    apr_initialize();

    fprintf(stderr,
            "This program won't work fully on this platform because there is no "
            "support for threads.\n");
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Failed.\nCould not initialize\n");
        exit(-1);
    }
    rv = apr_atomic_init(context);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Failed.\nCould not initialize atomics\n");
        exit(-1);
    }
    rv = check_basic_atomics(&y);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    return 0;
}
#else /* !APR_HAS_THREADS */

void * APR_THREAD_FUNC thread_func_mutex(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func_atomic(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func_none(apr_thread_t *thd, void *data);

apr_thread_mutex_t *thread_lock;
volatile long x = 0; /* mutex locks */
volatile long z = 0; /* no locks */
int value = 0;
apr_status_t exit_ret_val = 123; /* just some made up number to check on later */

#define NUM_THREADS 50
#define NUM_ITERATIONS 20000
void * APR_THREAD_FUNC thread_func_mutex(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < NUM_ITERATIONS; i++) {
        apr_thread_mutex_lock(thread_lock);
        x++;
        apr_thread_mutex_unlock(thread_lock);
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
} 

void * APR_THREAD_FUNC thread_func_atomic(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < NUM_ITERATIONS ; i++) {
        apr_atomic_inc(&y);
        apr_atomic_add(&y, 2);
        apr_atomic_dec(&y);
        apr_atomic_dec(&y);
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
}

void * APR_THREAD_FUNC thread_func_none(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < NUM_ITERATIONS ; i++) {
        z++;
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
}

int main(int argc, char**argv)
{
    apr_thread_t *t1[NUM_THREADS];
    apr_thread_t *t2[NUM_THREADS];
    apr_status_t r1[NUM_THREADS]; 
    apr_status_t r2[NUM_THREADS]; 
    apr_status_t s1[NUM_THREADS]; 
    apr_status_t s2[NUM_THREADS];
    apr_status_t rv;
    int i;
    int mutex;

    apr_initialize();

    if (argc == 2 && argv[1][0] == 'm') {
        mutex = 1;
    }
    else {
        mutex = 0;
    }

    printf("APR Atomic Test\n===============\n\n");
#if !(defined WIN32) && !(defined NETWARE) && !(defined __MVS__) && !(defined DARWIN)
    pthread_setconcurrency(8);
#endif
    printf("%-60s", "Initializing the context"); 
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Failed.\nCould not initialize\n");
        exit(-1);
    }
    printf("OK\n");

    if (mutex == 1) {
        printf("%-60s", "Initializing the lock"); 
        rv = apr_thread_mutex_create(&thread_lock, APR_THREAD_MUTEX_DEFAULT,
                                     context);
        if (rv != APR_SUCCESS) {
            fflush(stdout);
            fprintf(stderr, "Failed\nCould not create lock\n");
            exit(-1);
        }
        printf("OK\n");
    }
    printf("%-60s", "Initializing the atomics"); 
    rv = apr_atomic_init(context);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    printf("OK\n");

    rv = check_basic_atomics(&y);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    apr_atomic_set(&y, 0);

    printf("%-60s", "Starting all the threads"); 
    for (i = 0; i < NUM_THREADS; i++) {
        r1[i] = apr_thread_create(&t1[i], NULL, 
                    (mutex == 1 ? thread_func_mutex : thread_func_atomic),
                    NULL, context);
        r2[i] = apr_thread_create(&t2[i], NULL, thread_func_none, NULL,
                                  context);
        if (r1[i] != APR_SUCCESS || r2[i] != APR_SUCCESS ) {
            fflush(stdout);
            fprintf(stderr, "Failed\nError starting thread in group %d\n",i);
            exit(-1);
        }
    }
    printf("OK\n");

    printf("%-60s\n", "Waiting for threads to exit");
    printf("%-60s", "(Note that this may take a while to complete.)");
    fflush(stdout);

    for (i = 0; i < NUM_THREADS; i++) {
        apr_thread_join(&s1[i], t1[i]);
        apr_thread_join(&s2[i], t2[i]);
        if (s1[i] != exit_ret_val || s2[i] != exit_ret_val) {
            fprintf(stderr, 
                    "Invalid return value\n"
                    "Got %d/%d, but expected %d for all \n",
                    s1[i], s2[i], exit_ret_val);
        }
    }
    printf("OK\n");

    if (mutex == 1) {
        printf("%-60s", "Checking if mutex locks worked"); 
        if (x != NUM_THREADS * NUM_ITERATIONS) {
            fflush(stdout);
            fprintf(stderr, 
                    "No!\nThe locks didn't work?? x = %ld instead of %ld\n",
                    x,
                    (long)NUM_THREADS * NUM_ITERATIONS);
        }
        else {
            printf("OK\n");
        }
    }
    else {
        printf("%-60s", "Checking if atomic worked"); 
        if (apr_atomic_read(&y) != NUM_THREADS * NUM_ITERATIONS) {
            fflush(stdout);
            fprintf(stderr, 
                    "No!\nThe atomics didn't work?? y = %ld instead of %ld\n",
                    (long)apr_atomic_read(&y),
                    (long)NUM_THREADS * NUM_ITERATIONS);
        }
        else {
            printf("OK\n");
        }
    }
    printf("%-60s", "Checking if nolock worked"); 
    if (z != NUM_THREADS * NUM_ITERATIONS) {
        fflush(stdout);
        fprintf(stderr, 
                "no surprise\n"
                "The no-locks didn't work. z = %ld instead of %ld\n", 
                z, 
                (long)NUM_THREADS * NUM_ITERATIONS);
    }
    else {
        printf("OK\n");
    }

    return 0;
}

#endif /* !APR_HAS_THREADS */
