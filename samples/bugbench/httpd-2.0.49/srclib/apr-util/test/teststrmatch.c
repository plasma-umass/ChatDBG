/* Copyright 2002-2004 The Apache Software Foundation
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

#include "apr.h"
#include "apr_general.h"
#include "apr_strmatch.h"
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include "apr_want.h"


int main (void)
{
    apr_pool_t *pool;
    const apr_strmatch_pattern *pattern;
    const apr_strmatch_pattern *pattern_nocase;
    const apr_strmatch_pattern *pattern_onechar;
    const apr_strmatch_pattern *pattern_zero;
    const char *input1 = "string that contains a patterN...";
    const char *input2 = "string that contains a pattern...";
    const char *input3 = "pattern at the start of a string";
    const char *input4 = "string that ends with a pattern";
    const char *input5 = "patter\200n not found, negative chars in input";
    const char *input6 = "patter\200n, negative chars, contains pattern...";

    (void) apr_initialize();
    apr_pool_create(&pool, NULL);

    printf("testing pattern precompilation...");
    pattern = apr_strmatch_precompile(pool, "pattern", 1);
    if (!pattern) {
        printf("FAILED\n");
        exit(1);
    }
    pattern_nocase = apr_strmatch_precompile(pool, "pattern", 0);
    if (!pattern_nocase) {
        printf("FAILED\n");
        exit(1);
    }
    pattern_onechar = apr_strmatch_precompile(pool, "g", 0);
    if (!pattern_onechar) {
        printf("FAILED\n");
        exit(1);
    }
    pattern_zero = apr_strmatch_precompile(pool, "", 0);
    if (!pattern_zero) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing invalid match...");
    if (apr_strmatch(pattern, input1, strlen(input1)) != NULL) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing valid match...");
    if (apr_strmatch(pattern, input2, strlen(input2)) != input2 + 23) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing single-character match...");
    if (apr_strmatch(pattern_onechar, input1, strlen(input1)) != input1 + 5) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing zero-length pattern...");
    if (apr_strmatch(pattern_zero, input1, strlen(input1)) != input1) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing inexact-case match...");
    if (apr_strmatch(pattern_nocase, input1, strlen(input1)) != input1 + 23) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing match at start of string...");
    if (apr_strmatch(pattern, input3, strlen(input3)) != input3) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing match at end of string...");
    if (apr_strmatch(pattern, input4, strlen(input4)) != input4 + 24) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing invalid match with negative chars in input string...");
    if (apr_strmatch(pattern, input5, strlen(input5)) != NULL) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing valid match with negative chars in input string...");
    if (apr_strmatch(pattern, input6, strlen(input6)) != input6 + 35) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    exit(0);
}
