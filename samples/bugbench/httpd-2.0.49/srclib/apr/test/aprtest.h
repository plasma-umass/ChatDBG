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
#include "apr_strings.h"

#ifndef APR_TEST_PREFIX
#define APR_TEST_PREFIX ""
#endif

#define APR_TEST_BEGIN(rv, desc, op) \
    fprintf(stdout, "%s%.*s ", APR_TEST_PREFIX desc,                  \
            strlen(desc) < 37 ? (int)(40 - strlen(desc)) : 3,         \
            "........................................");              \
    APR_TEST_MORE(rv, op)

#define APR_TEST_MORE(rv, op) \
    if ((rv = (op)) != APR_SUCCESS) {                                 \
        char msgbuf[256];                                             \
        fprintf (stdout, "Failed\n");                                 \
        fprintf (stderr, "Error (%d): %s\n%s", rv, #op,               \
                 apr_strerror(rv, msgbuf, sizeof(msgbuf)));           \
        exit(-1); }

#define APR_TEST_END(rv, op) \
    APR_TEST_MORE(rv, op)                                             \
    fprintf(stdout, "OK\n");

#define APR_TEST_SUCCESS(rv, desc, op) \
    APR_TEST_BEGIN(rv, desc, op)                                      \
    fprintf(stdout, "OK\n");

#define APR_TEST_INITIALIZE(rv, pool) \
    APR_TEST_SUCCESS(rv, "Initializing", apr_initialize());           \
    atexit(apr_terminate);                                            \
    APR_TEST_SUCCESS(rv, "Creating context",                          \
                     apr_pool_create(&pool, NULL));

