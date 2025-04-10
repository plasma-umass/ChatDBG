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
#include "apr_rmm.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_time.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#if APR_HAS_SHARED_MEMORY

#define FRAG_SIZE 10
#define FRAG_COUNT 10
#define SHARED_SIZE (apr_size_t)(FRAG_SIZE * FRAG_COUNT * sizeof(char*))

static apr_status_t test_rmm(apr_pool_t *parpool)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_shm_t *shm;
    apr_rmm_t *rmm;
    apr_size_t size, fragsize;
    apr_rmm_off_t *off;
    int i;

    rv = apr_pool_create(&pool, parpool);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Error creating child pool\n");
        return rv;
    }

    /* We're going to want 10 blocks of data from our target rmm. */
    size = SHARED_SIZE + apr_rmm_overhead_get(FRAG_COUNT);
    printf("Creating anonymous shared memory (%"
           APR_SIZE_T_FMT " bytes).....", size); 
    rv = apr_shm_create(&shm, size, NULL, pool);
    if (rv != APR_SUCCESS) { 
        fprintf(stderr, "Error allocating shared memory block\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("Creating rmm segment.............................");
    rv = apr_rmm_init(&rmm, NULL, apr_shm_baseaddr_get(shm), size,
                      pool);

    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Error allocating rmm..............\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    fragsize = SHARED_SIZE / FRAG_COUNT;
    printf("Creating each fragment of size %" APR_SIZE_T_FMT "................",
           fragsize);
    off = apr_palloc(pool, FRAG_COUNT * sizeof(apr_rmm_off_t));
    for (i = 0; i < FRAG_COUNT; i++) {
        off[i] = apr_rmm_malloc(rmm, fragsize);
    } 
    fprintf(stdout, "OK\n");

    printf("Setting each fragment to a unique value..........");
    for (i = 0; i < FRAG_COUNT; i++) {
        int j;
        char **c = apr_rmm_addr_get(rmm, off[i]);
        for (j = 0; j < FRAG_SIZE; j++, c++) {
            *c = apr_itoa(pool, i + j);
        }
    } 
    fprintf(stdout, "OK\n");

    printf("Checking each fragment for its unique value......");
    for (i = 0; i < FRAG_COUNT; i++) {
        int j;
        char **c = apr_rmm_addr_get(rmm, off[i]);
        for (j = 0; j < FRAG_SIZE; j++, c++) {
            char *d = apr_itoa(pool, i + j);
            if (strcmp(*c, d) != 0) {
                return APR_EGENERAL;
            }
        }
    } 
    fprintf(stdout, "OK\n");

    printf("Freeing each fragment............................");
    for (i = 0; i < FRAG_COUNT; i++) {
        rv = apr_rmm_free(rmm, off[i]);
        if (rv != APR_SUCCESS) {
            return rv;
        }
    } 
    fprintf(stdout, "OK\n");

    printf("Creating one large segment.......................");
    off[0] = apr_rmm_calloc(rmm, SHARED_SIZE);
    fprintf(stdout, "OK\n");

    printf("Setting large segment............................");
    for (i = 0; i < FRAG_COUNT * FRAG_SIZE; i++) {
        char **c = apr_rmm_addr_get(rmm, off[0]);
        c[i] = apr_itoa(pool, i);
    }
    fprintf(stdout, "OK\n");

    printf("Freeing large segment............................");
    apr_rmm_free(rmm, off[0]);
    fprintf(stdout, "OK\n");

    printf("Creating each fragment of size %" APR_SIZE_T_FMT " (again)........",
           fragsize);
    for (i = 0; i < FRAG_COUNT; i++) {
        off[i] = apr_rmm_malloc(rmm, fragsize);
    } 
    fprintf(stdout, "OK\n");

    printf("Freeing each fragment backwards..................");
    for (i = FRAG_COUNT - 1; i >= 0; i--) {
        rv = apr_rmm_free(rmm, off[i]);
        if (rv != APR_SUCCESS) {
            return rv;
        }
    } 
    fprintf(stdout, "OK\n");

    printf("Creating one large segment (again)...............");
    off[0] = apr_rmm_calloc(rmm, SHARED_SIZE);
    fprintf(stdout, "OK\n");

    printf("Freeing large segment............................");
    apr_rmm_free(rmm, off[0]);
    fprintf(stdout, "OK\n");

    printf("Destroying rmm segment...........................");
    rv = apr_rmm_destroy(rmm);
    if (rv != APR_SUCCESS) {
        printf("FAILED\n");
        return rv;
    }
    printf("OK\n");

    printf("Destroying shared memory segment.................");
    rv = apr_shm_destroy(shm);
    if (rv != APR_SUCCESS) {
        printf("FAILED\n");
        return rv;
    }
    printf("OK\n");

    apr_pool_destroy(pool);

    return APR_SUCCESS;
}


int main(void)
{
    apr_status_t rv;
    apr_pool_t *pool;
    char errmsg[200];

    apr_initialize();
    
    printf("APR RMM Memory Test\n");
    printf("======================\n\n");

    printf("Initializing the pool............................"); 
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        printf("could not initialize pool\n");
        exit(-1);
    }
    printf("OK\n");

    rv = test_rmm(pool);
    if (rv != APR_SUCCESS) {
        printf("Anonymous shared memory test FAILED: [%d] %s\n",
               rv, apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-2);
    }
    printf("RMM test passed!\n");

    return 0;
}

#else /* APR_HAS_SHARED_MEMORY */
#error shmem is not supported on this platform
#endif /* APR_HAS_SHARED_MEMORY */

