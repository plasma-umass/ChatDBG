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

#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "apr_strings.h"
#include "test_apr.h"

static void format_arg(char *str, char option, const char *arg)
{
    if (arg) {
        apr_snprintf(str, 8196, "%soption: %c with %s\n", str, option, arg);
    }
    else {
        apr_snprintf(str, 8196, "%soption: %c\n", str, option);
    }
}

static void unknown_arg(void *str, const char *err, ...)
{
    va_list va;

    va_start(va, err);
    apr_vsnprintf(str, 8196, err, va);
    va_end(va);
}

static void no_options_found(CuTest *tc)
{
    int largc = 5;
    const char * const largv[] = {"testprog", "-a", "-b", "-c", "-d"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
   
    while (apr_getopt(opt, "abcd", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            default:
                format_arg(str, data, optarg);
        }
    }
    CuAssertStrEquals(tc, "option: a\n"
                          "option: b\n"
                          "option: c\n"
                          "option: d\n", str);
}

static void no_options(CuTest *tc)
{
    int largc = 5;
    const char * const largv[] = {"testprog", "-a", "-b", "-c", "-d"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "efgh", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "testprog: illegal option -- a\n", str);
}

static void required_option(CuTest *tc)
{
    int largc = 3;
    const char * const largv[] = {"testprog", "-a", "foo"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a:", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "option: a with foo\n", str);
}

static void required_option_notgiven(CuTest *tc)
{
    int largc = 2;
    const char * const largv[] = {"testprog", "-a"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a:", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "testprog: option requires an argument -- a\n", str);
}

static void optional_option(CuTest *tc)
{
    int largc = 3;
    const char * const largv[] = {"testprog", "-a", "foo"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a::", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "option: a with foo\n", str);
}

static void optional_option_notgiven(CuTest *tc)
{
    int largc = 2;
    const char * const largv[] = {"testprog", "-a"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a::", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
#if 0
/*  Our version of getopt doesn't allow for optional arguments.  */
    CuAssertStrEquals(tc, "option: a\n", str);
#endif
    CuAssertStrEquals(tc, "testprog: option requires an argument -- a\n", str);
}

CuSuite *testgetopt(void)
{
    CuSuite *suite = CuSuiteNew("Getopt");

    SUITE_ADD_TEST(suite, no_options);
    SUITE_ADD_TEST(suite, no_options_found);
    SUITE_ADD_TEST(suite, required_option);
    SUITE_ADD_TEST(suite, required_option_notgiven);
    SUITE_ADD_TEST(suite, optional_option);
    SUITE_ADD_TEST(suite, optional_option_notgiven);

    return suite;
}
