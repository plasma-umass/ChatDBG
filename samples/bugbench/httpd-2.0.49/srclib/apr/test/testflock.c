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

/*
 * USAGE
 *
 * Start one process, no args, and place it into the background. Start a
 * second process with the "-r" switch to attempt a read on the file
 * created by the first process.
 *
 * $ ./testflock &
 * ...messages...
 * $ ./testflock -r
 * ...messages...
 *
 * The first process will sleep for 30 seconds while holding a lock. The
 * second process will attempt to grab it (non-blocking) and fail. It
 * will then grab it with a blocking scheme. When the first process' 30
 * seconds are up, it will exit (thus releasing its lock). The second
 * process will acquire the lock, then exit.
 */

#include "apr_pools.h"
#include "apr_file_io.h"
#include "apr_time.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "apr_strings.h"

#include <stdlib.h>
#include <stdio.h>

const char *testfile = "testfile.tmp";

static apr_pool_t *pool = NULL;

static void errmsg(const char *msg)
{
    if (pool != NULL)
        apr_pool_destroy(pool);
    fprintf(stderr, msg);
    exit(1);
}

static void errmsg2(const char *msg, apr_status_t rv)
{
    char *newmsg;
    char errstr[120];

    apr_strerror(rv, errstr, sizeof errstr);
    newmsg = apr_psprintf(pool, "%s: %s (%d)\n",
                          msg, errstr, rv);
    errmsg(newmsg);
    exit(1);
}

static void do_read(void)
{
    apr_file_t *file;
    apr_status_t status;

    if (apr_file_open(&file, testfile, APR_WRITE,
                 APR_OS_DEFAULT, pool) != APR_SUCCESS)
        errmsg("Could not open test file.\n");
    printf("Test file opened.\n");

    status = apr_file_lock(file, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK);
    if (!APR_STATUS_IS_EAGAIN(status)) {
        char msg[200];
        errmsg(apr_psprintf(pool, "Expected APR_EAGAIN. Got %d: %s.\n",
                            status, apr_strerror(status, msg, sizeof(msg))));
    }
    printf("First attempt: we were properly locked out.\nWaiting for lock...");
    fflush(stdout);

    if (apr_file_lock(file, APR_FLOCK_EXCLUSIVE) != APR_SUCCESS)
        errmsg("Could not establish lock on test file.");
    printf(" got it.\n");

    (void) apr_file_close(file);
    printf("Exiting.\n");
}

static void do_write(void)
{
    apr_file_t *file;
    apr_status_t rv;

    if (apr_file_open(&file, testfile, APR_WRITE|APR_CREATE, APR_OS_DEFAULT,
                 pool) != APR_SUCCESS)
        errmsg("Could not create file.\n");
    printf("Test file created.\n");

    if ((rv = apr_file_lock(file, APR_FLOCK_EXCLUSIVE)) != APR_SUCCESS)
        errmsg2("Could not lock the file", rv);
    printf("Lock created.\nSleeping...");
    fflush(stdout);

    apr_sleep(apr_time_from_sec(30));

    (void) apr_file_close(file);
    printf(" done.\nExiting.\n");
}

int main(int argc, const char * const *argv)
{
    int reader = 0;
    apr_status_t status;
    char optchar;
    const char *optarg;
    apr_getopt_t *opt;

    if (apr_initialize() != APR_SUCCESS)
        errmsg("Could not initialize APR.\n");
    atexit(apr_terminate);

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS)
        errmsg("Could not create global pool.\n");

    if (apr_getopt_init(&opt, pool, argc, argv) != APR_SUCCESS)
        errmsg("Could not parse options.\n");

    while ((status = apr_getopt(opt, "rf:", &optchar, &optarg)) == APR_SUCCESS) {
        if (optchar == 'r')
            ++reader;
        else if (optchar == 'f')
            testfile = optarg;
    }
    if (status != APR_SUCCESS && status != APR_EOF) {
        char msgbuf[80];

        fprintf(stderr, "error: %s\n",
                apr_strerror(status, msgbuf, sizeof msgbuf));
        exit(1);
    }

    if (reader)
        do_read();
    else
        do_write();

    return 0;
}
