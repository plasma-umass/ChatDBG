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

static void msgput(int boxnum, char *msg)
{
    fprintf(stdout, "Producer: Sending message to box %d\n", boxnum);
    apr_cpystrn(boxes[boxnum].msg, msg, strlen(msg));
    boxes[boxnum].msgavail = 1;
}

int main(void)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_shm_t *shm;
    int i;
    char errmsg[200];

    apr_initialize();
    
    printf("APR Shared Memory Test: PRODUCER\n");

    printf("Initializing the pool............................"); 
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        printf("could not initialize pool\n");
        exit(-1);
    }
    printf("OK\n");

    printf("Producer attaching to name-based shared memory....");
    rv = apr_shm_attach(&shm, SHARED_FILENAME, pool);
    if (rv != APR_SUCCESS) {
        printf("Producer unable to attach to name-based shared memory "
               "segment: [%d] %s \n", rv,
               apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-2);
    }
    printf("OK\n");

    boxes = apr_shm_baseaddr_get(shm);

    /* produce messages on all of the boxes, in descending order */
    for (i = N_BOXES - 1; i > 0; i--) {
        msgput(i, "Sending a message\n");
        apr_sleep(apr_time_from_sec(1));
    }

    printf("Producer detaching from name-based shared memory....");
    rv = apr_shm_detach(shm);
    if (rv != APR_SUCCESS) {
        printf("Producer unable to detach from name-based shared memory "
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

