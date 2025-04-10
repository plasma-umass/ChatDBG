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
#include "apr_global_mutex.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"


#define MAX_ITER 4000
#define MAX_COUNTER (MAX_ITER * 4)

apr_global_mutex_t *global_lock;
apr_pool_t *pool;
int *x;

static int make_child(apr_proc_t **proc, apr_pool_t *p)
{
    int i = 0;
    *proc = apr_pcalloc(p, sizeof(**proc));

    /* slight delay to allow things to settle */
    apr_sleep (1);

    if (apr_proc_fork(*proc, p) == APR_INCHILD) {
        while (1) {
            apr_global_mutex_lock(global_lock); 
            if (i == MAX_ITER) {
                apr_global_mutex_unlock(global_lock); 
                exit(1);
            }
            i++;
            (*x)++;
            apr_global_mutex_unlock(global_lock); 
        }
        exit(1);
    }
    return APR_SUCCESS;
}

static apr_status_t test_exclusive(const char *lockname)
{
    apr_proc_t *p1, *p2, *p3, *p4;
    apr_status_t s1, s2, s3, s4;
 
    printf("Exclusive lock test\n");
    printf("%-60s", "    Initializing the lock");
    s1 = apr_global_mutex_create(&global_lock, lockname, APR_LOCK_DEFAULT, pool);
 
    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");
 
    printf("%-60s", "    Starting all of the processes");
    fflush(stdout);
    s1 = make_child(&p1, pool);
    s2 = make_child(&p2, pool);
    s3 = make_child(&p3, pool);
    s4 = make_child(&p4, pool);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS ||
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");
 
    printf("%-60s", "    Waiting for processes to exit");
    s1 = apr_proc_wait(p1, NULL, NULL, APR_WAIT);
    s2 = apr_proc_wait(p2, NULL, NULL, APR_WAIT);
    s3 = apr_proc_wait(p3, NULL, NULL, APR_WAIT);
    s4 = apr_proc_wait(p4, NULL, NULL, APR_WAIT);
    printf("OK\n");
 
    if ((*x) != MAX_COUNTER) {
        fprintf(stderr, "Locks don't appear to work!  x = %d instead of %d\n",
                (*x), MAX_COUNTER);
    }
    else {
        printf("Test passed\n");
    }
    return APR_SUCCESS;
}

int main(int argc, const char * const *argv)
{
    apr_status_t rv;
    char errmsg[200];
    const char *lockname = NULL;
    const char *shmname = "shm.file";
    apr_getopt_t *opt;
    char optchar;
    const char *optarg;
    apr_shm_t *shm;

    printf("APR Proc Mutex Test\n==============\n\n");
        
    apr_initialize();
    atexit(apr_terminate);

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS)
        exit(-1);

    if ((rv = apr_getopt_init(&opt, pool, argc, argv)) != APR_SUCCESS) {
        fprintf(stderr, "Could not set up to parse options: [%d] %s\n",
                rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }
        
    while ((rv = apr_getopt(opt, "f:", &optchar, &optarg)) == APR_SUCCESS) {
        if (optchar == 'f') {
            lockname = optarg;
        }
    }

    if (rv != APR_SUCCESS && rv != APR_EOF) {
        fprintf(stderr, "Could not parse options: [%d] %s\n",
                rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }

    apr_shm_create(&shm, sizeof(int), shmname, pool);
    x = apr_shm_baseaddr_get(shm);

    if ((rv = test_exclusive(lockname)) != APR_SUCCESS) {
        fprintf(stderr,"Exclusive Lock test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-2);
    }
    
    return 0;
}

