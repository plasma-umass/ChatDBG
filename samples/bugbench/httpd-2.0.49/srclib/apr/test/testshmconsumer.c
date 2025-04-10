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

typedef struct mbox {
    char msg[1024]; 
    int msgavail; 
} mbox;
mbox *boxes;

#define N_BOXES 10
#define SHARED_SIZE (apr_size_t)(N_BOXES * sizeof(mbox))
#define SHARED_FILENAME "/tmp/apr.testshm.shm"

static void msgwait(int sleep_sec, int first_box, int last_box)
{
    int i;
    apr_time_t start = apr_time_now();
    apr_interval_time_t sleep_duration = apr_time_from_sec(sleep_sec);
    while (apr_time_now() - start < sleep_duration) {
        for (i = first_box; i < last_box; i++) {
            if (boxes[i].msgavail) {
                fprintf(stdout, "Consumer: received a message in box %d, message was: %s\n", 
                        i, boxes[i].msg); 
                boxes[i].msgavail = 0; /* reset back to 0 */
            }
        }
        apr_sleep(apr_time_from_sec(1));
    }
    fprintf(stdout, "Consumer: done waiting on mailboxes...\n");
}

int main(void)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_shm_t *shm;
    char errmsg[200];

    apr_initialize();
    
    printf("APR Shared Memory Test: CONSUMER\n");

    printf("Initializing the pool............................"); 
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        printf("could not initialize pool\n");
        exit(-1);
    }
    printf("OK\n");

    printf("Consumer attaching to name-based shared memory....");
    rv = apr_shm_attach(&shm, SHARED_FILENAME, pool);
    if (rv != APR_SUCCESS) {
        printf("Consumer unable to attach to name-based shared memory "
               "segment: [%d] %s \n", rv,
               apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-2);
    }
    printf("OK\n");

    boxes = apr_shm_baseaddr_get(shm);

    /* consume messages on all of the boxes */
    msgwait(30, 0, N_BOXES); /* wait for 30 seconds for messages */

    printf("Consumer detaching from name-based shared memory....");
    rv = apr_shm_detach(shm);
    if (rv != APR_SUCCESS) {
        printf("Consumer unable to detach from name-based shared memory "
               "segment: [%d] %s \n", rv,
               apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-3);
    }
    printf("OK\n");

    return 0;
}

#else /* APR_HAS_SHARED_MEMORY */

int main(void)
{
    printf("APR SHMEM test not run!\n");
    printf("shmem is not supported on this platform\n"); 
    return -1;
}

#endif /* APR_HAS_SHARED_MEMORY */

