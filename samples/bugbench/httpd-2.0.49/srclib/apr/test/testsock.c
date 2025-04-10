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
#include <signal.h>
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"

#define STRLEN 15

static int run_basic_test(apr_pool_t *context)
{
    apr_procattr_t *attr1 = NULL;
    apr_procattr_t *attr2 = NULL;
    apr_proc_t proc1;
    apr_proc_t proc2;
    apr_status_t s1;
    apr_status_t s2;
    const char *args[2];

    fprintf(stdout, "Creating children to run network tests.......\n");
    s1 = apr_procattr_create(&attr1, context);
    s2 = apr_procattr_create(&attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem creating proc attrs\n");
        exit(-1);
    }

    args[0] = apr_pstrdup(context, "server");
    args[1] = NULL; 
    s1 = apr_proc_create(&proc1, "./server", args, NULL, attr1, context);

    /* Sleep for 5 seconds to ensure the server is setup before we begin */
    apr_sleep(5000000);
    args[0] = apr_pstrdup(context, "client");
    s2 = apr_proc_create(&proc2, "./client", args, NULL, attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem spawning new process\n");
        exit(-1);
    }

    while ((s1 = apr_proc_wait(&proc1, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE && 
           (s2 = apr_proc_wait(&proc2, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE) {
        continue;
    }

    if (s1 == APR_SUCCESS) {
        apr_proc_kill(&proc2, SIGTERM);
        while (apr_proc_wait(&proc2, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    else {
        apr_proc_kill(&proc1, SIGTERM);
        while (apr_proc_wait(&proc1, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    fprintf(stdout, "Network test completed.\n");   

    return 1;
}

static int run_sendfile(apr_pool_t *context, int number)
{
    apr_procattr_t *attr1 = NULL;
    apr_procattr_t *attr2 = NULL;
    apr_proc_t proc1;
    apr_proc_t proc2;
    apr_status_t s1;
    apr_status_t s2;
    const char *args[4];

    fprintf(stdout, "Creating children to run network tests.......\n");
    s1 = apr_procattr_create(&attr1, context);
    s2 = apr_procattr_create(&attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem creating proc attrs\n");
        exit(-1);
    }

    args[0] = apr_pstrdup(context, "sendfile");
    args[1] = apr_pstrdup(context, "server");
    args[2] = NULL; 
    s1 = apr_proc_create(&proc1, "./sendfile", args, NULL, attr1, context);

    /* Sleep for 5 seconds to ensure the server is setup before we begin */
    apr_sleep(5000000);
    args[1] = apr_pstrdup(context, "client");
    switch (number) {
        case 0: {
            args[2] = apr_pstrdup(context, "blocking");
            break;
        }
        case 1: {
            args[2] = apr_pstrdup(context, "nonblocking");
            break;
        }
        case 2: {
            args[2] = apr_pstrdup(context, "timeout");
            break;
        }
    }
    args[3] = NULL;
    s2 = apr_proc_create(&proc2, "./sendfile", args, NULL, attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem spawning new process\n");
        exit(-1);
    }

    while ((s1 = apr_proc_wait(&proc1, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE && 
           (s2 = apr_proc_wait(&proc2, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE) {
        continue;
    }

    if (s1 == APR_SUCCESS) {
        apr_proc_kill(&proc2, SIGTERM);
        while (apr_proc_wait(&proc2, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    else {
        apr_proc_kill(&proc1, SIGTERM);
        while (apr_proc_wait(&proc1, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    fprintf(stdout, "Network test completed.\n");   

    return 1;
}

int main(int argc, char *argv[])
{
    apr_pool_t *context = NULL;

    fprintf(stdout, "Initializing.........");
    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    atexit(apr_terminate);

    fprintf(stdout, "Creating context.......");
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Could not create context\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "This test relies on the process test working.  Please\n");
    fprintf(stdout, "run that test first, and only run this test when it\n");
    fprintf(stdout, "completes successfully.  Alternatively, you could run\n");
    fprintf(stdout, "server and client by yourself.\n");
    run_basic_test(context);
    run_sendfile(context, 0);
    run_sendfile(context, 1);
    run_sendfile(context, 2);

    return 0;
}
