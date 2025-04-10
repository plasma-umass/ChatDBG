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

#include "test_apr.h"

/* Top-level pool which can be used by tests. */
apr_pool_t *p;

void apr_assert_success(CuTest* tc, const char* context, apr_status_t rv)
{
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, context);
    }

    if (rv != APR_SUCCESS) {
        char buf[STRING_MAX], ebuf[128];
        sprintf(buf, "%s (%d): %s\n", context, rv,
                apr_strerror(rv, ebuf, sizeof ebuf));
        CuFail(tc, buf);
    }
}

static const struct testlist {
    const char *testname;
    CuSuite *(*func)(void);
} tests[] = {
    {"teststr", teststr},
    {"testtime", testtime},
    {"testvsn", testvsn},
    {"testipsub", testipsub},
    {"testmmap", testmmap},
    {"testud", testud},
    {"testtable", testtable},
    {"testhash", testhash},
    {"testsleep", testsleep},
    {"testpool", testpool},
    {"testfmt", testfmt},
    {"testfile", testfile},
    {"testfileinfo", testfileinfo},
    {"testpipe", testpipe},
    {"testdup", testdup},
    {"testdir", testdir},
    {"testrand", testrand},
    {"testdso", testdso},
    {"testoc", testoc},
    {"testsockets", testsockets},
    {"testsockopt", testsockopt},
    {"testproc", testproc},
    {"testprocmutex", testprocmutex},
    {"testpoll", testpoll},
    {"testlock", testlock},
    {"testthread", testthread},
    {"testargs", testgetopt},
    {"testnames", testnames},
    {"testuser", testuser},
    {"testpath", testpath},
    {"testenv", testenv},
    {"LastTest", NULL}
};

int main(int argc, char *argv[])
{
    CuSuiteList *alltests = NULL;
    CuString *output = CuStringNew();
    int i;
    int exclude = 0;
    int list_provided = 0;

    apr_initialize();
    atexit(apr_terminate);

    CuInit(argc, argv);

    apr_pool_create(&p, NULL);

    /* see if we're in exclude mode, see if list of testcases provided */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) {
            continue;
        }
        if (!strcmp(argv[i], "-x")) {
            exclude = 1;
            continue;
        }
        if (!strcmp(argv[i], "-l")) {
            for (i = 0; tests[i].func != NULL; i++) {
                printf("%s\n", tests[i].testname);
            }
            exit(0);
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "invalid option: `%s'\n", argv[i]);
            exit(1);
        }
        list_provided = 1;
    }

    if (!list_provided) {
        /* add everything */
        alltests = CuSuiteListNew("All APR Tests");
        for (i = 0; tests[i].func != NULL; i++) {
            CuSuiteListAdd(alltests, tests[i].func());
        }
    }
    else if (exclude) {
        /* add everything but the tests listed */
        alltests = CuSuiteListNew("Partial APR Tests");
        for (i = 0; tests[i].func != NULL; i++) {
            int this_test_excluded = 0;
            int j;

            for (j = 1; j < argc && !this_test_excluded; j++) {
                if (!strcmp(argv[j], tests[i].testname)) {
                    this_test_excluded = 1;
                }
            }
            if (!this_test_excluded) {
                CuSuiteListAdd(alltests, tests[i].func());
            }
        }
    }
    else {
        /* add only the tests listed */
        alltests = CuSuiteListNew("Partial APR Tests");
        for (i = 1; i < argc; i++) {
            int j;
            int found = 0;

            if (argv[i][0] == '-') {
                continue;
            }
            for (j = 0; tests[j].func != NULL; j++) {
                if (!strcmp(argv[i], tests[j].testname)) {
                    CuSuiteListAdd(alltests, tests[j].func());
                    found = 1;
                }
            }
            if (!found) {
                fprintf(stderr, "invalid test name: `%s'\n", argv[i]);
                exit(1);
            }
        }
    }
    
    CuSuiteListRunWithSummary(alltests);
    i = CuSuiteListDetails(alltests, output);
    printf("%s\n", output->buffer);

    return i > 0 ? 1 : 0;
}

