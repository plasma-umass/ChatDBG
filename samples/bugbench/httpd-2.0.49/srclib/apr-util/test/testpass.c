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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "apr_errno.h"
#include "apr_strings.h"
#include "apr_file_io.h"
#include "apr_thread_proc.h"
#include "apr_md5.h"

static struct {
    const char *password;
    const char *hash;
} passwords[] =
{
/*
  passwords and hashes created with Apache's htpasswd utility like this:
  
  htpasswd -c -b passwords pass1 pass1
  htpasswd -b passwords pass2 pass2
  htpasswd -b passwords pass3 pass3
  htpasswd -b passwords pass4 pass4
  htpasswd -b passwords pass5 pass5
  htpasswd -b passwords pass6 pass6
  htpasswd -b passwords pass7 pass7
  htpasswd -b passwords pass8 pass8
  (insert Perl one-liner to convert to initializer :) )
 */
    {"pass1", "1fWDc9QWYCWrQ"},
    {"pass2", "1fiGx3u7QoXaM"},
    {"pass3", "1fzijMylTiwCs"},
    {"pass4", "nHUYc8U2UOP7s"},
    {"pass5", "nHpETGLGPwAmA"},
    {"pass6", "nHbsbWmJ3uyhc"},
    {"pass7", "nHQ3BbF0Y9vpI"},
    {"pass8", "nHZA1rViSldQk"}
};
static int num_passwords = sizeof(passwords) / sizeof(passwords[0]);

static void check_rv(apr_status_t rv)
{
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "bailing\n");
        exit(1);
    }
}

static void test(void)
{
    int i;

    for (i = 0; i < num_passwords; i++) {
        apr_status_t rv = apr_password_validate(passwords[i].password,
                                                passwords[i].hash);
        assert(rv == APR_SUCCESS);
    }
}

#if APR_HAS_THREADS

static void * APR_THREAD_FUNC testing_thread(apr_thread_t *thd,
                                             void *data)
{
    int i;

    for (i = 0; i < 100; i++) {
        test();
    }
    return APR_SUCCESS;
}

static void thread_safe_test(apr_pool_t *p)
{
#define NUM_THR 20
    apr_thread_t *my_threads[NUM_THR];
    int i;
    apr_status_t rv;
    
    for (i = 0; i < NUM_THR; i++) {
        rv = apr_thread_create(&my_threads[i], NULL, testing_thread, NULL, p);
        check_rv(rv);
    }

    for (i = 0; i < NUM_THR; i++) {
        apr_thread_join(&rv, my_threads[i]);
    }
}
#endif

int main(void)
{
    apr_status_t rv;
    apr_pool_t *p;

    rv = apr_initialize();
    check_rv(rv);
    rv = apr_pool_create(&p, NULL);
    check_rv(rv);
    atexit(apr_terminate);

    /* before creating any threads, test it first just to check
     * for problems with the test driver
     */
    printf("dry run\n");
    test();

#if APR_HAS_THREADS
    printf("thread-safe test\n");
    thread_safe_test(p);
#endif
    
    return 0;
}
